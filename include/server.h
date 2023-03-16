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
    void handle_client(SOCKET clientSocket);
    bool mpIsChatServer;

protected:
    SOCKET mServerSocket;
    sockaddr_in mAddress;

private:
    std::unordered_set<SOCKET> mActiveConnections;
    std::mutex mActiveConnectionsMutex;
    std::vector<std::thread> mThreads;
};