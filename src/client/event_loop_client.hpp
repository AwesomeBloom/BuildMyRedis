//
// @brief: A Client with Event Loop Implementation
// @birth: created by Tianyi on 2024/01/16
// @version: V0.0.1
//

#pragma once

#include "base_client.hpp"

class EventLoopClient : public BaseClient {
protected:
    static const size_t K_MAX_MSG = 4096;

    static int32_t sent_req(int fd, const char *text) {
        auto len = static_cast<uint32_t>(strlen(text));
        if (len > K_MAX_MSG) {
            msg("message is too long");
            return -1;
        }

        char wbuf[4 + K_MAX_MSG];
        memcpy(wbuf, &len, 4);
        memcpy(&wbuf[4], text, len);

        return write_all(fd, wbuf, 4 + len);
    }

    static int32_t read_res(int fd) {
        char rbuf[4 + K_MAX_MSG + 1];
        errno = 0;

        // read length
        int32_t err = read_full(fd, rbuf, 4);
        if (err) {
            if (errno == 0) {
                msg("EOF");
            } else {
                msg("read() error");
            }
            return err;
        }

        uint32_t len = 0;
        memcpy(&len, rbuf, 4);
        if (len > K_MAX_MSG) {
            msg("message is too long");
            return -1;
        }

        // reply body
        err = read_full(fd, &rbuf[4], len);
        if (err) {
            msg("read() error");
            return err;
        }

        rbuf[4 + len] = '\0';
        printf("server says: %s\n", &rbuf[4]);
        return 0;
    }

public:
    __attribute__((unused)) int work(const char* msg) override {
        return 0;
    }

    int work(const char* msg[], size_t n_msg) {
        curFd = socket(AF_INET, SOCK_STREAM, 0);
        if (curFd < 0) {
            die("socket()");
        }

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
        int rv = connect(curFd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr));
        if (rv) {
            die("connect");
        }

        for (size_t i = 0; i < n_msg; ++i) {
            int32_t err = sent_req(curFd, msg[i]);
            if (err) {
                close(curFd);
                return 0;
            }
        }

        for (size_t i = 0; i < n_msg; ++i) {
            int32_t err = read_res(curFd);
            if (err) {
                close(curFd);
                return 0;
            }
        }

        close(curFd);
        return 0;
    }
};