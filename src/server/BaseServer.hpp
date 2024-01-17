//
// @brief: Base server (Baseclass of all Server)
// @birth: created by Tianyi on 2024/01/08
// @version: v0.0.1
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <cassert>
#include <iostream>

#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "../utils/OwnRedisException.hpp"

// A basic server
class BaseServer {
protected:
    static const int BUF_SIZE = 64;

    // current socket
    int curFd{-1};

    // report and die
    static void die(const char *msg) {
        const int err = errno;
        fprintf(stderr, "[%d] %s\n", err, msg);
        abort();
    }

    // report msg to stderr
    static void msg(const char *msg) {
        fprintf(stderr, "%s\n", msg);
    }

    // read n chars;
    static int32_t read_full(int fd, char *buf, size_t n) {
        while (n > 0) {
            ssize_t rv = read(fd, buf, n);
            if (rv <= 0) {
                return -1;
            }
            assert(static_cast<size_t>(rv) <= n);
            n -= static_cast<size_t>(rv);
            buf += rv;
        }
        return 0;
    }

    // write all stuff in buffer
    static int32_t write_all(int fd, const char* buf, size_t n) {
        while(n > 0) {
            ssize_t rv = write(fd, buf, n);
            if (rv <= 0) {
                return -1;
            }
            assert(static_cast<size_t>(rv) <= n);
            n -= static_cast<size_t>(rv);
            buf += rv;
        }
        return 0;
    }

    // react to client
    virtual int32_t perform_request(int connfd) {
        return 0;
    }

public:
    ~BaseServer() {
        if (curFd >= 0) {
            close(curFd);
        }
    }

    // main work function
    virtual int work() {
        return 0;
    }
};