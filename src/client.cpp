#include "../include/client.h"

Client::Client()
    : mIsReadyToSend(false), mIsRunning(true), mClientSocket(INVALID_SOCKET)
{
}

Client::~Client()
{
    if (mClientSocket != INVALID_SOCKET) {
        closesocket(mClientSocket);
    }

    if (mReceiveThread.joinable()) {
        mReceiveThread.join();
    }

    WSACleanup();
}

bool Client::init() {
    // Init win sock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }

    // init client socket
    mClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (mClientSocket == INVALID_SOCKET) {
        std::cerr << "Socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // init server details
    mServerAddress.sin_family = AF_INET;
    mServerAddress.sin_port = htons(12345);
    mServerAddress.sin_addr.s_addr = inet_addr(LOCALHOST);

    return true;
}

bool Client::connecting()
{
    int result = connect(mClientSocket, (sockaddr*)&mServerAddress, sizeof(mServerAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(mClientSocket);
        WSACleanup();
        return false;
    }

    std::cout << "Connected to server." << std::endl;

    mReceiveThread = std::thread(&Client::receiveMessages, this, std::ref(mIsRunning));
    return true;
}

void Client::receiveMessages(std::atomic<bool>& mIsRunning) {
    char buffer[BUFFER_SIZE];
    int result = 0;
    while (mIsRunning) {

        result = recv(mClientSocket, buffer, sizeof(buffer), 0);
        if (result > 0) 
        {
            std::string message(buffer, result);
            std::cout << message << std::endl;

            if (message == "OK")
            {
                std::lock_guard<std::mutex> lock(mReadyMutex);
                mIsReadyToSend = true;
                mReadyCondVar.notify_one();
                break;
            }
        } else if (result == 0) {
            std::cout << "Server disconnected." << std::endl;
            mIsRunning = false;
        } else {
            std::cerr << "Error: " << WSAGetLastError() << std::endl;
            mIsRunning = false;
        }
    }
}

void Client::sendMessage()
{
    std::string message;
    do {
        std::getline(std::cin, message);

        int result = send(mClientSocket, message.c_str(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            mIsRunning = false;
            closesocket(mClientSocket);
            WSACleanup();
            return;
        }
        
    } while (message != "quit");
}

void Client::sendFile(std::string filename)
{
    std::string message = "file " + filename;
    int result = send(mClientSocket, message.c_str(), message.size(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        mIsRunning = false;
        closesocket(mClientSocket);
        WSACleanup();
        return;
    }

    std::unique_lock<std::mutex> lock(mReadyMutex);
    mReadyCondVar.wait(lock ,[this]{return mIsReadyToSend; });
    mIsReadyToSend = false;

    std::ifstream inputFile(filename, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Failed to open file." << std::endl;
        mIsRunning = false;
        return;
    }

    char buffer[FILE_SIZE];
    while (inputFile) {

        inputFile.read(buffer, FILE_SIZE);
        int bytesRead = inputFile.gcount();

        int bytesSent = 0;
        while (bytesSent < bytesRead) {
            result = send(mClientSocket, buffer + bytesSent, bytesRead - bytesSent, 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
                mIsRunning = false;
                closesocket(mClientSocket);
                WSACleanup();
                return;
            }
            bytesSent += result;
        }
    }

    inputFile.close();
    mIsRunning = false;
}

int main(int argc, char** argv) {

    if (!(argc == 2 && (std::strcmp(argv[1], "-m") == 0)) && !(argc == 3 && (std::strcmp(argv[1], "-f") == 0)))
    {
        std::cerr << "Usage: " << argv[0] << " [-m | -f filename]" << std::endl;
        return 1;  
    }

    // Parse command-line arguments
    bool isSendingMessage = false;
    bool isSendingFile = false;
    std::string filename;
    
    std::string arg = argv[1];
    if (arg == "-m") {
        isSendingMessage = true;
    } else if (arg == "-f") {
        isSendingFile = true;
        filename = argv[2];
    }
    Client client;

    if(!client.init())
    {
        std::cout << "Failed to init client" << std::endl;
        return -1;
    }

    if(!client.connecting())
    {
        std::cout << "Failed to connect to server" << std::endl;
    }


    // Send and receive data from the server
    std::string message;
    if (isSendingMessage)
    {
        client.sendMessage();
    } 
    else if (isSendingFile)
    {
        client.sendFile(filename);
    }

    return 0;
}
