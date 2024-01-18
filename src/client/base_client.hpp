//
// @brief: Base Client (Baseclass of all Client)
// @birth: created by Tianyi on 2024/01/08
// @version: v0.0.1
//

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cassert>
#include <iostream>

#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#include "../utils/own_redis_exception.hpp"

// A base client
class BaseClient {
protected:
    static const int BUF_SIZE = 64;
    int curFd{-1};

    static void msg(const char *msg) {
        fprintf(stderr, "%s\n", msg);
    }

    // report and die
    static void die(const char *msg) {
        const int err = errno;
        fprintf(stderr, "[%d] %s\n", err, msg);
        abort();
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

    // perform query
    virtual int32_t query(int fd, const char* text) {
        return 0;
    }

public:

    ~BaseClient() {
        if (curFd >= 0) {
            close(curFd);
        }
    }

    // main work function
    virtual int work(const char *msg) {
        return 0;
    }
};