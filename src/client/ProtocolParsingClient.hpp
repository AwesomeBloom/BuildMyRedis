//
//@brief: A client with protocol parsing (toy protocol)
//@birth: created by Tianyi on 2024/01/08
//@version: V0.0.1
//

#pragma once

#include "BaseClient.hpp"

class ProtocolParsingClient : public BaseClient {
    static const size_t K_MAX_MSG = 4096;

    int32_t query(int fd, const char* text) {
        auto len = static_cast<uint32_t>(strlen(text));

        if (len > K_MAX_MSG) {
            return -1;
        }

        char wbuf[4 + K_MAX_MSG];
        memcpy(wbuf, &len, 4);  // little endian
        memcpy(&wbuf[4], text, len);

        if (int32_t err = write_all(fd, wbuf, 4 + len)) {
            return err;
        }

        // header: 4 bytes
        char rbuf[4 + K_MAX_MSG + 1];
        errno = 0;
        int32_t err = read_full(fd, rbuf, 4);
        if (err) {
            if (errno == 0) {
                msg("EOF");
            } else {
                msg("read() error");
            }
            return err;
        }

        memcpy(&len, rbuf, 4);
        if (len > K_MAX_MSG) {
            msg("message is too long!");
            return -1;
        }

        // reply body
        err = read_full(fd, &rbuf[4], len);
        if (err) {
            msg("read() error");
            return err;
        }

        // do sth
        rbuf[4 + len] = '\0';
        printf("server says: %s\n", &rbuf[4]);
        return 0;
    }

    static inline void handle_error() {}

public:
    int work(const char* msg) override {
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
        for (int i = 0; i < 5; ++i) {
            int32_t err = query(curFd, msg);
            if (err) {
                close(curFd);
                curFd = -1;
                throw BaseException("Query Error!");
                return err;
            }
        }
        
        return 0;
    }

};