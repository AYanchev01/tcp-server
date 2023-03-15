
#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <WinSock2.h>

#define LOCALHOST "127.0.0.1"
constexpr size_t BUFFER_SIZE = 1024;
constexpr size_t FILE_SIZE = 200000;


class Client
{
public:
    Client();
    ~Client();

    bool init();
    bool connecting();
    void sendMessage();
    void sendFile(std::string fileName);
private:
    std::mutex mReadyMutex;
    std::condition_variable mReadyCondVar;
    bool mIsReadyToSend;
    std::atomic<bool> mIsRunning;
    void receiveMessages(std::atomic<bool>& running);
    std::thread mReceiveThread;
protected:
    SOCKET mClientSocket;
    sockaddr_in mServerAddress;
};