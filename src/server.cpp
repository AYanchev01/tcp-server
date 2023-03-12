#include <iostream>
#include <thread>
#include <vector>
#include <unordered_set>
#include "../include/utils.h"
#include <WinSock2.h>


// Define the maximum number of clients that can connect to the server
constexpr int MAX_CLIENTS = 5;
std::unordered_set<SOCKET> active_connections;

void read_message(int clientSocket)
{
    char buffer[1024];
    int result = 0;
    // Loop to receive data from the client
    do {
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (result > 0) {
            // Data was received, process it here
            std::cout << "Received data from : " << clientSocket << " " << std::string(buffer, result) << std::endl;

            // Echo the data back to the client
            send(clientSocket, buffer, result, 0);
        }
        else if (result == 0) {
            // Client disconnected
            active_connections.erase(clientSocket);
            std::cout << "Client disconnected." << std::endl;
        }
        else {
            // Error occurred
            active_connections.erase(clientSocket);
            std::cerr << "Error: " << WSAGetLastError() << std::endl;
        }
    } while (result > 0);
}

void read_file(int clientSocket)
{
    int len = 0;
    int result = 0;
    char buffer[1024];


	int r_size = recv(clientSocket, &len, sizeof(len), 0);
	if(r_size < sizeof(len)) {
		return;
	}

	char *filename = new char[len];

	recv(clientSocket, filename, len, 0);

    // Loop to receive data from the client
    do {
        result = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (result > 0) {
            // Data was received, process it here
            std::cout << "Received file name from : " << clientSocket << " " << std::string(buffer, result) << std::endl;

            std::ofstream file(filename, std::ios::binary);

            uint8_t* content = new uint8_t[50000];
	        result = recv(clientSocket, (char*)content, 50000, 0);

	        file.write((const char*)content, result);

	        std::cout << "File received: " << filename << std::endl;

	        delete [] filename;
	        delete [] content;
        }
        else if (result == 0) {
            // Client disconnected
            active_connections.erase(clientSocket);
            std::cout << "Client disconnected." << std::endl;
        }
        else {
            // Error occurred
            active_connections.erase(clientSocket);
            std::cerr << "Error: " << WSAGetLastError() << std::endl;
        }
    } while (result > 0);

	std::ofstream file(filename, std::ios::binary);


}

void clientHandler(SOCKET clientSocket) {
    char buffer[1024];
    int command = 0;
    int r_size = recv(clientSocket, (char*)&command, sizeof(command), 0);
    if(r_size < sizeof(command))
    {
        std::cout << "Error receiving command" << std::endl;
        return;
    }


    if((Command)command == Command::MESSAGE)
    {
        read_message(clientSocket);
    }
    else if((Command)command == Command::FILE)
    {
        read_file(sockID);
    }

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
    while (active_connections.size() < MAX_CLIENTS) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        // Spawn a thread to handle the client
        active_connections.insert(clientSocket);
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

