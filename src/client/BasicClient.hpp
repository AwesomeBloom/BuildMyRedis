// 
// @brief: Basic client
// @birth: created by Tianyi on 2024/01/08
// @version: v0.0.1
// 

#pragma once

#include "BaseClient.hpp"

// A basic client
class BasicClient : public BaseClient{
public:

    ~BasicClient() {
        if (curFd >= 0) {
            close(curFd);
        }
    }

    // main work function
    int work(const char* msg) {
        curFd = socket(AF_INET, SOCK_STREAM, 0);
        if (curFd < 0) {
            die("socket()");
        }

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
        int rv = connect(curFd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
        if (rv) {
            die("connect");
        }

        // send
        write(curFd, msg, strlen(msg));

        // receive
        std::string rbuf(BUF_SIZE, ' ');
        ssize_t n = read(curFd, const_cast<char *>(rbuf.c_str()), sizeof(rbuf.c_str()) - 1);
        if (n < 0) {
            die("read");
        }

        fprintf(stdout, "server says: %s \n", rbuf.c_str());
        close(curFd);

        return 0;
    }
};