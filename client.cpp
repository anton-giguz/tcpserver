/*
** client.cpp -- TCP client for calculating average
*/

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"


class ClientException : public std::runtime_error {
  public:
    ClientException(const std::string& what) : std::runtime_error(what) {}
    ClientException(const std::string& prefix, const std::string& what) : std::runtime_error(prefix + ": " + what) {}
};


class Client
{
    int sockfd;

  public:
    Client(const char *host, const char *port);
    ~Client() { close(sockfd); }
    int32_t run(const char *const *vals);
};


Client::Client(const char *host, const char *port) {
    addrinfo hints, *paddrinfo;
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // fill paddrinfo
    int errcode = getaddrinfo(host, port, &hints, &paddrinfo);
    if (errcode != 0)
        throw ClientException("getaddrinfo", gai_strerror(errcode));

    // loop through the list of addresses
    bool success = false;
    for (auto p = paddrinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd != -1) { // socket created successfully
            if (connect(sockfd, p->ai_addr, p->ai_addrlen) != -1) { // connect was successful, break from loop
                success = true;
                break;
            } else {
                close(sockfd);
            }
        }
    }
    freeaddrinfo(paddrinfo); // list no longer needed
    if (!success)
        throw ClientException("Cannot connect to any address");
}


void randompause() {
    int ms = rand() / (RAND_MAX / 50);
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}


int32_t Client::run(const char *const *vals) {
    for (; *vals != NULL; vals++) {
        int32_t value = atol(*vals);
        if (value == 0) break;

        randompause();

        if (!sendint32(sockfd, value))
            throw ClientException("Cannot send");
    }

    if (!sendint32(sockfd, 0))
        throw ClientException("Cannot send");

    int32_t result = 0;
    if (!recvint32(sockfd, &result))
        throw ClientException("Cannot receive");

    return result;
}


int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " host port [value...]\n";
        return EXIT_FAILURE;
    }
    const char *host = argv[1], *port = argv[2];
    const char *const *vals = &argv[3];

    try {
        Client client(host, port);
        int32_t result = client.run(vals);
        std::cout << result << std::endl;
        return EXIT_SUCCESS;
    }
    catch (ClientException& e) {
        std::cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
