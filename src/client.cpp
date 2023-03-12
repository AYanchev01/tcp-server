#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <WinSock2.h>

constexpr size_t BUFFER_SIZE = 1024;

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

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create a TCP socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Connect to the server
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(12345);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    result = connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress));
    if (result == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    // Send and receive data from the server
    std::string message;
    if (isSendingMessage)
    {
        do {
            // Read a message from the user
            std::cout << "> ";
            std::getline(std::cin, message);

            // Send the message to the server
            result = send(clientSocket, message.c_str(), message.length(), 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                closesocket(clientSocket);
                WSACleanup();
                return 1;
            }
            
        } while (message != "quit");
    } 
    else if (isSendingFile)
    {
        // Send the filename to the server
        message = "file " + filename;
        int result = send(clientSocket, message.c_str(), message.size(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        // Wait for acknowledgement from the server
        char buffer[BUFFER_SIZE];
        result = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (result == SOCKET_ERROR) {
            // Handle error...
        }
        // Check if the server sent an acknowledgement
        if (std::string(buffer, result) == "OK") {
            // Send the file data to the server
            std::ifstream inputFile(filename, std::ios::binary);
            if (inputFile) {
                char buffer[BUFFER_SIZE];
                do {
                    inputFile.read(buffer, BUFFER_SIZE);
                    int bytesRead = inputFile.gcount();
                    if (bytesRead > 0) {
                        result = send(clientSocket, buffer, bytesRead, 0);
                        if (result == SOCKET_ERROR) {
                            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                            closesocket(clientSocket);
                            WSACleanup();
                            return 1;
                        }
                    }
                } while (inputFile.good());
                // Close the file
                inputFile.close();
            } else {
                std::cout << "Failed to open file." << std::endl;
                return 1;
            }
        } else {
            // Handle error...
        }

    }

    // Close the client socket
    closesocket(clientSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
