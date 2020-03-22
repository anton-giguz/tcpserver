/*
** server.cpp -- TCP server for calculating average
*/

#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"


namespace cond {
    std::mutex mutex;
    std::condition_variable var;
    bool connected = false;
    int fd = -1;
    unsigned int tasksdone = 0;
}

namespace all {
    std::mutex mutex;
    int64_t sum = 0;
    unsigned int count = 0;
}


class ServerException : public std::runtime_error {
  public:
    ServerException(const std::string& what) : std::runtime_error(what) {}
    ServerException(const std::string& prefix, const std::string& what) : std::runtime_error(prefix + ": " + what) {}
};


class Server
{
    int sockfd;
    const int BACKLOG = 10;

  public:
    Server(const char *port);
    ~Server() { close(sockfd); }
    int32_t run();
};


Server::Server(const char *port) {
    addrinfo hints, *paddrinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    // fill paddrinfo
    int errcode = getaddrinfo(NULL, port, &hints, &paddrinfo);
    if (errcode != 0)
        throw ServerException("getaddrinfo", gai_strerror(errcode));

    // loop through the list of addresses
    bool success = false;
    for (auto p = paddrinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd != -1) { // socket created successfully
            int opt = 1;
            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt); // prevent "Address already in use"
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) != -1) { // bind was successful, break from loop
                success = true;
                break;
            } else {
                close(sockfd);
            }
        }
    }
    freeaddrinfo(paddrinfo); // list no longer needed
    if (!success)
        throw ServerException("Cannot bind to any address");
}


// divide with rounding
int32_t divide(int64_t sum, unsigned int count) {
    if (count == 0) return 0;
    return sum >= 0 ? (sum + count / 2) / count : -((-sum + count / 2) / count);
}


// read data from an accepted connection and send back the result when 0 is received
void session(int fd) {
    try {
    	int64_t sum = 0;
    	unsigned int count = 0;
        while (true) {
            int32_t value = 0;
            if (!recvint32(fd, &value))
                throw ServerException("Cannot receive");
            if (value == 0) break;

            sum += value;
            count++;

            std::lock_guard<std::mutex> guard(all::mutex);
            all::sum += value;
            all::count++;
        }

        if (count != 0) {
            if (!sendint32(fd, divide(sum, count)))
                throw ServerException("Cannot send");
        }
    }
    catch (ServerException& e) {
        std::cerr << e.what() << "\n";
    }

    close(fd);
    {
        std::lock_guard<std::mutex> guard(cond::mutex);
        cond::tasksdone++;
    }
    cond::var.notify_one(); // notify dispatcher when session is finished
}


// wait for incoming connections
void listening(int sockfd) {
    while (true) {
        int fd = accept(sockfd, NULL, NULL);
        if (fd != -1) {
            {
                std::lock_guard<std::mutex> guard(cond::mutex);
                cond::connected = true;
                cond::fd = fd;
            }
            cond::var.notify_one(); // notify dispatcher when connection is accepted
        }
    }
}


// wait for notifications from created threads
void dispatching(int sockfd) {
    std::thread listener(listening, sockfd);
    std::vector<std::thread> workers;

    while (true) {
        std::unique_lock<std::mutex> lock(cond::mutex);
        cond::var.wait(lock);

        if (cond::connected) { // new connection
            workers.push_back(std::thread(session, cond::fd));
            cond::connected = false;
        }

        if (cond::tasksdone == workers.size()) break; // all sessions finished
    }
    
    listener.detach();
    for (auto& worker : workers) worker.join();
}


int32_t Server::run() {
    if (listen(sockfd, BACKLOG) == -1)
        throw ServerException("listen", strerror(errno));

    std::thread dispatcher(dispatching, sockfd);
    dispatcher.join();

    std::lock_guard<std::mutex> guard(all::mutex);
    if (all::count == 0)
        throw ServerException("No values received");
    return divide(all::sum, all::count);
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " port\n";
        return EXIT_FAILURE;
    }
    const char *port = argv[1];

    try {
        Server server(port);
        std::cout << "Starting server...\n";

        int32_t result = server.run();
        std::ofstream file("result.txt");
        file << result << std::endl;

        if (file.fail()) {
            std::cerr << "Cannot write result to file\n";
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    catch (ServerException& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
