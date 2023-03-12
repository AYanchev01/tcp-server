#include <iostream>
#include <string>
#include "../include/utils.h"
#include <WinSock2.h>

int sending(int socketID, Request request)
{
    int result;

    if(request.command == Command::MESSAGE)
    {
        result = send(socketID, (const char*)&request.command, sizeof(request.command), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(socketID);
            WSACleanup();
            return 1;
        }
        result = send(socketID, request.data.c_str(), request.data.length(),0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(socketID);
            WSACleanup();
            return 1;
        }
    }
    else if(request.command == Command::FILE)
    {
        std::ifstream file(request.data);

        if(file.is_open())
        {
            file.seekg(0, file.end);
            int length = file.tellg();
            file.seekg(0, file.beg);

            char * buffer = new char[length];
            file.read(buffer, length);

            send(socketID, (const char*)&request.command, sizeof(request.command), 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                closesocket(socketID);
                WSACleanup();
                return 1;
            }
            send(socketID, request.data.c_str(), request.data.length(), 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                closesocket(socketID);
                WSACleanup();
                return 1;
            }
            send(socketID, buffer, length, 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                closesocket(socketID);
                WSACleanup();
                return 1;
            }
            delete [] buffer;
        }
    }
}

int main(int argc, char **args) {
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
    do {
        if(std::strcmp(args[1], "-m") == 0)
        {
            Request request;
            request.command == Command::MESSAGE;
            request.data = args[2];
            sending(clientSocket, request);
        }
        else if(std::strcmp(args[1], "-f") == 0)
        {
            Request request;
            request.command = Command::FILE;
            request.data = args[2];
            sending(clientSocket, request);
        }
    } while (message != "quit");

    // Close the client socket
    closesocket(clientSocket);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
