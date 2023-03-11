#include <iostream>
#include <thread>
#include <vector>
#include <WinSock2.h>


// Define the maximum number of clients that can connect to the server
constexpr int MAX_CLIENTS = 5;

void clientHandler(SOCKET clientSocket) {
    char buffer[1024];
    int result = 0;

    // Loop to receive data from the client
    do {
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (result > 0) {
            // Data was received, process it here
            std::cout << "Received data: " << std::string(buffer, result) << std::endl;

            // Echo the data back to the client
            send(clientSocket, buffer, result, 0);
        }
        else if (result == 0) {
            // Client disconnected
            std::cout << "Client disconnected." << std::endl;
        }
        else {
            // Error occurred
            std::cerr << "Error: " << WSAGetLastError() << std::endl;
        }
    } while (result > 0);

    // Close the client socket
    closesocket(clientSocket);
}


int main() {
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create a TCP socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Bind the socket to a port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    result = bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    result = listen(serverSocket, SOMAXCONN);
    if (result == SOCKET_ERROR) {
        std::cerr << "listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port 12345..." << std::endl;

    std::vector<std::thread> threads;

    // Accept incoming connections and spawn threads to handle them
    while (threads.size() < MAX_CLIENTS) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Spawn a thread to handle the client
        threads.emplace_back(clientHandler, clientSocket);
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        thread.join();
    }

    // Close the server socket
    closesocket(serverSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}

