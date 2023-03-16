#pragma once

#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <cstring>
#include <unordered_set>
#include <sstream>
#include <mutex>
#include <WinSock2.h>

constexpr int MAX_CLIENTS = 10;
constexpr size_t BUFFER_SIZE = 1024;


class Server
{
public:
    Server();
    ~Server();

    bool init();
    void accept_connections();
    bool mpIsChatServer;

protected:
    SOCKET mServerSocket;
    sockaddr_in mAddress;

private:
    std::unordered_set<SOCKET> mActiveConnections;
    std::mutex mActiveConnectionsMutex;
    std::vector<std::thread> mThreads;
    void handle_client(SOCKET clientSocket);
    void handle_chat_message(SOCKET clientSocket, const std::string& message);
    void handle_file_transfer(SOCKET clientSocket, const std::string& fileName);
    bool is_sending_file(const std::string& message);
};