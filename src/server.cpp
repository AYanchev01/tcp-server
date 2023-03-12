#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <WinSock2.h>

// Define a mutex and condition variable to coordinate the threadpool
std::mutex mtx;
std::condition_variable cv;
bool is_running = true;

// Define a queue to hold incoming client connections
std::queue<SOCKET> client_queue;

// Define a vector to hold worker threads in the threadpool
std::vector<std::thread> thread_pool;

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
            std::cout << "Received data from : " << clientSocket << " " << std::string(buffer, result) << std::endl;

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

    // Notify the threadpool that this thread is finished and can be reused
    {
        std::lock_guard<std::mutex> lock(mtx);
        thread_pool.erase(std::remove_if(thread_pool.begin(), thread_pool.end(),
            [](const std::thread& t) { return t.get_id() == std::this_thread::get_id(); }), thread_pool.end());
    }
    cv.notify_one();
}

// Define a function to initialize the threadpool
void initThreadPool() {
    // Start with a single worker thread in the pool
    thread_pool.emplace_back([&]() {
        while (is_running) {
            // Wait for a client connection to become available
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() { return !client_queue.empty() || !is_running; });

            // If the threadpool is no longer running, exit the loop
            if (!is_running) {
                return;
            }

            // Get the next client connection from the queue and process it
            SOCKET clientSocket = client_queue.front();
            client_queue.pop();
            lock.unlock();
            clientHandler(clientSocket);
        }
    });
}

// Define a function to stop the threadpool
void stopThreadPool() {
    // Set the is_running flag to false to stop the worker threads
    is_running = false;

    // Notify all worker threads that they should exit
    cv.notify_all();

    // Join all worker threads to ensure they have finished
    for (auto& thread : thread_pool) {
        thread.join();
    }

    // Clear the threadpool vector
    thread_pool.clear();
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


    initThreadPool();
    //std::vector<std::thread> threads;

    // Main loop to accept incoming connections and add them to the queue
    while (true) {
        // Accept incoming connections and add them to the queue
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            // Determine the number of free threads in the threadpool
            int free_threads = 0;
            {
                std::lock_guard<std::mutex> lock(mtx);
                free_threads = thread_pool.size() - client_queue.size();
            }

            // If there are no free threads, add a new thread to the threadpool
            if (free_threads == 0) {
                std::thread new_thread([&]() {
                    clientHandler(clientSocket);
                });
                new_thread.detach();
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    thread_pool.emplace_back(std::move(new_thread));
                }
            }
            // Otherwise, add the new client connection to the queue
            else
            {
                std::lock_guard<std::mutex> lock(mtx);
                client_queue.push(clientSocket);
            }
            cv.notify_one();
        }
    }

    // Stop the threadpool
    stopThreadPool();

    // Cleanup Winsock
    // ...

    return 0;
}

