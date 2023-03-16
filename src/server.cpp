#include "../include/server.h"


Server::Server()
    : mpIsChatServer(false), mServerSocket(INVALID_SOCKET)
{
}

Server::~Server()
{
    if (mServerSocket != INVALID_SOCKET) {
        closesocket(mServerSocket);
    }

    for (auto& thread : mThreads) {
        thread.join();
    }

    closesocket(mServerSocket);
    WSACleanup();
}

bool Server::init()
{
    // Initialize win sock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }

    // Create a TCP socket
    mServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mServerSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Bind the socket to a port
    mAddress.sin_family = AF_INET;
    mAddress.sin_port = htons(12345);
    mAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    result = bind(mServerSocket, (sockaddr*)&mAddress, sizeof(mAddress));

    if (result == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(mServerSocket);
        WSACleanup();
        return false;
    }

    // Listen for incoming connections
    result = listen(mServerSocket, MAX_CLIENTS);
    if (result == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(mServerSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Server listening on port 12345..." << std::endl;
    return true;
}

void Server::handle_client(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int result = 0;

    mActiveConnectionsMutex.lock();
    mActiveConnections.insert(clientSocket);
    mActiveConnectionsMutex.unlock();
    
    do {
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        
        if (result <= 0) {
            std::cerr << "Client[" << clientSocket << "] disconnected." << std::endl;
            break;
        }

        std::string message(buffer, result);

        if (is_sending_file(message)) {
            handle_file_transfer(clientSocket, message.substr(5));
        }
        else {
            handle_chat_message(clientSocket, message);
        }
        
    } while (result > 0);
    

    mActiveConnectionsMutex.lock();
    mActiveConnections.erase(clientSocket);
    mActiveConnectionsMutex.unlock();

    closesocket(clientSocket);
}

void Server::handle_chat_message(SOCKET clientSocket, const std::string& message) {
    if (mpIsChatServer) {
        std::unordered_set<SOCKET> connections = mActiveConnections;
        for (const auto& client : connections) {
            if (client != clientSocket) {
                std::ostringstream oss;
                oss << "User[" << clientSocket << "]: " << message;
                send(client, oss.str().c_str(), oss.str().size(), 0);
            }
        }
    }
    else {
        std::cout << "User[" << clientSocket << "]: " << message << std::endl;
    }
}

void Server::handle_file_transfer(SOCKET clientSocket, const std::string& filename) {
    char buffer[BUFFER_SIZE];

    // Send an acknowledgement message to the client
    const char* ackMessage = "OK";
    int ok_status = send(clientSocket, ackMessage, strlen(ackMessage), 0);
    if (ok_status == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    std::ofstream outputFile(filename, std::ios::binary);
    int bytesRead;

    do {
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            break;
        }

        outputFile.write(buffer, bytesRead);
    } while (bytesRead == BUFFER_SIZE);

    outputFile.close();
}

bool Server::is_sending_file(const std::string& message) {
    return message.substr(0, 5) == "file ";
}

void Server::accept_connections()
{
    // Accept incoming connections and spawn mThreads to handle them
    while (mActiveConnections.size() < MAX_CLIENTS) {
        SOCKET client_socket = accept(mServerSocket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Spawn a thread to handle the client
        mThreads.emplace_back(&Server::handle_client,this,client_socket);
    }
}


int main(int argc, char** argv) {

    Server server;
    if (!(argc == 2 && ((std::strcmp(argv[1], "-c") == 0) || (std::strcmp(argv[1], "-r") == 0))))
    {
        std::cerr << "Usage: .\\server.exe OPTION" << std::endl << std::endl;
        std::cerr << "  -r   start receiving mode" << std::endl;
        std::cerr << "  -c   start chat mode"  << std::endl;
        return 1;  
    }
    else if (std::strcmp(argv[1],"-c") == 0)
    {
        server.mpIsChatServer = true;
    }

    if (!server.init())
    {
        std::cout << "Failed to start server" << std::endl;
    }

    server.accept_connections();

    return 0;
}

