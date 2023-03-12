#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <WinSock2.h>


// Define the maximum number of clients that can connect to the server
constexpr int MAX_CLIENTS = 5;

constexpr size_t BUFFER_SIZE = 1024;

void clientHandler(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int result = 0;

    
    // Loop to receive data from the client
    do {
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        std::cout << "Received data from : " << clientSocket << " " << std::string(buffer, result) << std::endl;
        // Check if the client is sending a file
        bool isSendingFile = false;
        std::string filename;
        if (std::string(buffer).substr(0, 5) == "file ") {
        isSendingFile = true;
        filename = std::string(buffer).substr(5);
        }

        if (result > 0) {
            if (isSendingFile)
            {
                std::ofstream outputFile(filename, std::ios::binary);

                // Read file data from the client and write it to the file
                int bytesRead;
                do {
                    result = recv(clientSocket, buffer, BUFFER_SIZE, 0);
                    if (result == SOCKET_ERROR) {
                        std::cerr << "recv failed: " << WSAGetLastError() << std::endl;
                        closesocket(clientSocket);
                        WSACleanup();
                        return;
                    }
                    bytesRead = result;
                    outputFile.write(buffer, bytesRead);
                } while (bytesRead == BUFFER_SIZE);

                // Close the file
                outputFile.close();
            }
            else
            {
                // Data was received, process it here
                std::cout << "Received data from : " << clientSocket << " " << std::string(buffer, result) << std::endl;
            }
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
    result = listen(serverSocket, MAX_CLIENTS);
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

