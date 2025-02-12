#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <thread>
#include <ifaddrs.h> // For getifaddrs

std::string get_local_ipv6()
{
    struct ifaddrs *ifaddr, *ifa;
    int family;

    // Get the list of network interfaces
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Iterate through the list
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue; // Skip interfaces with no address
        }

        family = ifa->ifa_addr->sa_family;

        // Check for IPv6 addresses
        if (family == AF_INET6) {
            char addr[INET6_ADDRSTRLEN];
            struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)ifa->ifa_addr;

            // Convert the binary address to a human-readable string
            if (inet_ntop(AF_INET6, &in6->sin6_addr, addr, sizeof(addr)) != NULL) {
                std::string ret(addr);
                if (ret.find("::1") == 0)
                    continue;
                if (ret.find("fe80") == 0)
                    continue;
                std::cout << "Address " << ret << std::endl;
                return ret;
            }
        }
    }

    // Free the linked list
    freeifaddrs(ifaddr);
    throw std::runtime_error("No ipv6 address found");
}

void set_socket_timeout(int sockfd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;  // Seconds
    timeout.tv_usec = 0;       // Microseconds

    // Set receive timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("setsockopt (SO_RCVTIMEO)");
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        throw std::runtime_error("setsockopt (SO_SNDTIMEO)");
    }
}

void simpleDataExchange(int clientSocket, int serverSocket) {
    char buffer[128];
    const char* message = "Hello from client";

    // Send data from client to server
    std::thread t([clientSocket, serverSocket](){
        const char* message = "Hello from client";
        if (send(clientSocket, message, strlen(message), 0) < 0) {
            throw std::runtime_error("Failed to send data from client");
        }
    });

    // Receive data on server
    int len = recv(serverSocket, buffer, sizeof(buffer) - 1, 0);
    if (len < 0) {
        throw std::runtime_error("Failed to receive data on server");
    }
    buffer[len] = '\0';

    std::cout << "Received: " << buffer << std::endl;
    t.join();
}

void test1Ipv6(int port, int serverSocket_) {
   int c1 = socket(AF_INET6, SOCK_STREAM, 0);
    if (c1 < 0) {
        throw std::runtime_error("Failed to create IPv6 client socket");
    }

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, "::1", &serverAddr.sin6_addr);
    serverAddr.sin6_port = htons(port);

    if (connect(c1, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(c1);
        throw std::runtime_error("Failed to connect IPv6 client socket");
    }

    int s1 = accept(serverSocket_, nullptr, nullptr);
    if (s1 < 0) {
        close(c1);
        throw std::runtime_error("Failed to accept IPv6 server socket");
    }

    simpleDataExchange(c1, s1);

    close(c1);
    close(s1);
    std::cout << "done test1Ipv6" << std::endl;
}

void test1Both(int port, int serverSocket_) {
      std::cout << "test1Both(int port)" << std::endl;

        int c1 = socket(AF_INET, SOCK_STREAM, 0);
        if (c1 < 0) {
            throw std::runtime_error("Failed to create IPv4 client socket");
        }

        sockaddr_in serverAddr4{};
        serverAddr4.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &serverAddr4.sin_addr);
        serverAddr4.sin_port = htons(port);

        if (connect(c1, (sockaddr*)&serverAddr4, sizeof(serverAddr4)) < 0) {
            close(c1);
            throw std::runtime_error("Failed to connect IPv4 client socket");
        }
      std::cout << "done connect ipv4" << std::endl;
        int c2 = socket(AF_INET6, SOCK_STREAM, 0);
        if (c2 < 0) {
            close(c1);
            throw std::runtime_error("Failed to create IPv6 client socket");
        }


        sockaddr_in6 serverAddr6{};
        serverAddr6.sin6_family = AF_INET6;
        inet_pton(AF_INET6, "::1", &serverAddr6.sin6_addr);
        serverAddr6.sin6_port = htons(port);

        if (connect(c2, (sockaddr*)&serverAddr6, sizeof(serverAddr6)) < 0) {
            close(c1);
            close(c2);
            throw std::runtime_error("Failed to connect IPv6 client socket");
        }
        std::cout << "done connect ipv6" << std::endl;

        int s1 = accept(serverSocket_, nullptr, nullptr);
        if (s1 < 0) {
            close(c1);
            close(c2);
            throw std::runtime_error("Failed to accept server socket");
        }


        int s2 = accept(serverSocket_, nullptr, nullptr);
        if (s2 < 0) {
            close(c1);
            close(c2);
            close(s1);
            throw std::runtime_error("Failed to accept server socket");
        }
        std::cout << "done accept both" << std::endl;

        sockaddr_in6 addr{};
        socklen_t addrLen = sizeof(addr);
        getsockname(s1, (sockaddr*)&addr, &addrLen);

        if (addr.sin6_family != AF_INET6) {
            std::cout << "exchange s1 is ipv6" << std::endl;
            simpleDataExchange(c2, s1);
            std::cout << "done exchange s1 is ipv6" << std::endl;
            simpleDataExchange(c1, s2);
            std::cout << "done second exchange s1 is ipv6" << std::endl;
        }
        else {
            std::cout << "exchange s1 is ipv4" << std::endl;
            simpleDataExchange(c1, s1);
            std::cout << "done exchange s1 is ipv4" << std::endl;
            simpleDataExchange(c2, s2);
            std::cout << "done second exchange s1 is ipv4" << std::endl;
        }

        close(c1);
        close(c2);
        close(s1);
        close(s2);
}

void test1()
{
    std::cout << "create server socket " << std::endl;

    int serverSocket_ = socket(AF_INET6, SOCK_STREAM, 0);

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_addr = in6addr_any;
    serverAddr.sin6_port = 0; // Let OS choose a port

    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(serverSocket_, 5) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on server socket");
    }

    socklen_t addrLen = sizeof(serverAddr);
    getsockname(serverSocket_, (sockaddr*)&serverAddr, &addrLen);
    int port = ntohs(serverAddr.sin6_port);
    test1Ipv6(port, serverSocket_);
    test1Both(port, serverSocket_);
    close(serverSocket_);
}

void testIPv4Exchange(int port, int serverSocket_) {
   int c1 = socket(AF_INET, SOCK_STREAM, 0);
        if (c1 < 0) {
            throw std::runtime_error("Failed to create IPv4 client socket");
        }

        sockaddr_in serverAddr4{};
        serverAddr4.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &serverAddr4.sin_addr);
        serverAddr4.sin_port = htons(port);

        if (connect(c1, (sockaddr*)&serverAddr4, sizeof(serverAddr4)) < 0) {
            close(c1);
            throw std::runtime_error("Failed to connect IPv4 client socket");
        }

    int s1 = accept(serverSocket_, nullptr, nullptr);
    if (s1 < 0) {
        close(c1);
        throw std::runtime_error("Failed to accept IPv4 server socket");
    }

    simpleDataExchange(c1, s1);

    close(c1);
    close(s1);
    std::cout << "done test1Ipv4" << std::endl;
}

void testIPv6Exchange(int port, int serverSocket_) {
    int c1 = socket(AF_INET6, SOCK_STREAM, 0);
    if (c1 < 0) {
        throw std::runtime_error("Failed to create IPv6 client socket");
    }

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    std::string addr = get_local_ipv6();
    inet_pton(AF_INET6, addr.c_str(), &serverAddr.sin6_addr);
    serverAddr.sin6_port = htons(port);

    if (connect(c1, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(c1);
        throw std::runtime_error("Failed to connect IPv6 client socket");
    }

    int s1 = accept(serverSocket_, nullptr, nullptr);
    if (s1 < 0) {
        close(c1);
        throw std::runtime_error("Failed to accept IPv6 server socket");
    }

    simpleDataExchange(c1, s1);

    close(c1);
    close(s1);
    std::cout << "done test1Ipv6" << std::endl;
}


void test3() {
      std::cout << "create server socket " << std::endl;

    int serverSocket_ = socket(AF_INET6, SOCK_STREAM, 0);
    set_socket_timeout(serverSocket_, 5);
    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_addr = in6addr_any;
    serverAddr.sin6_port = 0; // Let OS choose a port

    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(serverSocket_, 5) < 0) {
        close(serverSocket_);
        throw std::runtime_error("Failed to listen on server socket");
    }

    socklen_t addrLen = sizeof(serverAddr);
    getsockname(serverSocket_, (sockaddr*)&serverAddr, &addrLen);
    int port = ntohs(serverAddr.sin6_port);

    std::cout << "test3 - ipv4 conn" << std::endl;
    testIPv4Exchange(port, serverSocket_);
    std::cout << "test3 - ipv 6 conn" << std::endl;
    testIPv6Exchange(port, serverSocket_);
    close(serverSocket_);
    std::cout << "Test3: OK" << std::endl;
}

int main() {
    test1();
    test3();
}

