#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>

int BUFFER_SIZE = 4;
int RESPONSE_SIZE = 8;


bool isPrime(int n) {
    if (n <= 1)
        return false;
    for (int i = 2; i <= std::sqrt(n); ++i) {
        if (n % i == 0)
            return false;
    }
    return true;
}


void workerThread(std::queue<int>& tasks, std::mutex& mutex, std::condition_variable& cv) {
    while (true) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [&]{ return !tasks.empty(); });
        
        int task = tasks.front();
        tasks.pop();
        
        lock.unlock();
        lock.release();
        cv.notify_one();
        
        int prime = 0;
        for (int i = 2, count = 0; count < task; ++i) {
            if (isPrime(i))
                ++count;
            if (count == task) {
                prime = i;
                break;
            }
        }
        
        lock.lock();
        std::cout << "Task: " << task << ", Prime: " << prime << std::endl;
        lock.unlock();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    
    int port = std::atoi(argv[1]);
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }
    
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to bind to port " << port << "." << std::endl;
        return 1;
    }
    
    if (listen(serverSocket, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on socket." << std::endl;
        return 1;
    }
    
    std::cout << "Server listening on port " << port << "." << std::endl;
    
    std::mutex mutex;
    std::condition_variable cv;
    std::queue<int> tasks;
    std::vector<std::thread> workers;
    

    for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
        workers.emplace_back(workerThread, std::ref(tasks), std::ref(mutex), std::ref(cv));
    }
    
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept client connection." << std::endl;
continue;
}

std::cout << "Accepted new client connection." << std::endl;
    

    int number;
    if (recv(clientSocket, &number, sizeof(number), 0) == -1) {
        std::cerr << "Failed to receive input number from client." << std::endl;
        close(clientSocket);
        continue;
    }
    
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(number);
    }
    
    cv.notify_one();
    

    int response = 0;
    for (int i = 2, count = 0; count < number; ++i) {
        if (isPrime(i))
            ++count;
        if (count == number) {
            response = i;
            break;
        }
    }
    
    if (send(clientSocket, &response, sizeof(response), 0) == -1) {
        std::cerr << "Failed to send response to client." << std::endl;
    }
    
    close(clientSocket);
}


for (auto& worker : workers) {
    worker.join();
}

close(serverSocket);

return 0;
