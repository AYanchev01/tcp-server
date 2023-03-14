#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <WinSock2.h>

constexpr size_t BUFFER_SIZE = 1024;
std::mutex readyMutex;
std::condition_variable readyCondVar;
bool isReadyToSend = false;

void receiveMessages(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    int result = 0;
    while (true) {
        // Receive data from the server
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (result > 0) {
            // Data was received, process it here
            std::string message(buffer, result);
            std::cout << message << std::endl;

            if (message == "OK")
            {
                std::lock_guard<std::mutex> lock(readyMutex);
                isReadyToSend = true;
                readyCondVar.notify_one();
                break;
            }
        } else if (result == 0) {
            // Server disconnected
            std::cout << "Server disconnected." << std::endl;
        } else {
            // Error occurred
            std::cerr << "Error: " << WSAGetLastError() << std::endl;
        }
    }
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


    std::thread receiveThread(receiveMessages, clientSocket);

    // Send and receive data from the server
    std::string message;
    if (isSendingMessage)
    {
        do {
            // Read a message from the user
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

        std::unique_lock<std::mutex> lock(readyMutex);
        readyCondVar.wait(lock ,[]{return isReadyToSend; });
        isReadyToSend = false;

        std::ifstream inputFile(filename, std::ios::binary);
        if (!inputFile) {
            std::cerr << "Failed to open file." << std::endl;
            return 1;
        }

        // Send the file in chunks
        char buffer[BUFFER_SIZE];
        while (inputFile) {
            // Read a chunk from the file
            inputFile.read(buffer, BUFFER_SIZE);
            int bytesRead = inputFile.gcount();

            // Send the chunk to the server
            int bytesSent = 0;
            while (bytesSent < bytesRead) {
                result = send(clientSocket, buffer + bytesSent, bytesRead - bytesSent, 0);
                if (result == SOCKET_ERROR) {
                    std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                    WSACleanup();
                    return 1;
                }
                bytesSent += result;
            }
        }

        // Close the file
        inputFile.close();
        std::cout << "got to balbal\n";
    } else {
        // Handle error...
    }


    // Wait for the receive thread to exit
    receiveThread.join();

    // Close the client socket
    closesocket(clientSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
