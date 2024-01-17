//
// @brief: Server with protocol parsing
// @birth: created by Tianyi on 2024/01/08
// @version: v0.0.1
//
#pragma once

#include "BaseServer.hpp"

// A server with protocol parsing
class ProtocolParsingServer : public BaseServer {
    static const size_t K_MAX_MSG = 4096;

    int32_t perform_request(int connfd) override {
        char rbuf[4 + K_MAX_MSG + 1];
        errno = 0;
        int32_t err = read_full(connfd, rbuf, 4);
        if (err) {
            if (errno == 0) {
                msg("EOF");
            } else {
                msg("read() error");
            }
            return err;
        }

        uint32_t len = 0;
        memcpy(&len, rbuf, 4);  // little endian
        if (len > K_MAX_MSG) {
            msg("message is too long");
            return -1;
        }

        err = read_full(connfd, &rbuf[4], len);
        if (err) {
            msg("read() error");
            return err;
        }

        rbuf[4 + len] = '\0';
        printf("client says: %s\n", &rbuf[4]);

        char reply[len + 1];
        sprintf(reply, "server received a message, length: %d", len);
        char wbuf[4 + sizeof(reply)];
        uint32_t len_reply = static_cast<uint32_t>(strlen(reply));
        memcpy(wbuf, &len_reply, 4);
        memcpy(&wbuf[4], reply, len_reply);
        return write_all(connfd, wbuf, 4 + len_reply);
    }

public:
    int work() override {
        curFd = socket(AF_INET, SOCK_STREAM, 0);
        if (curFd < 0) {
            die("socket()");
        }

        int val = 1;
        setsockopt(curFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

        // bind address and port
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(0);
        int rv = bind(curFd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr));
        if (rv) {
            die("bind()");
        }

        rv = listen(curFd, SOMAXCONN);
        if (rv) {
            die("listen()");
        }

        while (true) {
            // accept messages
            struct sockaddr_in client_addr = {};
            socklen_t socklen = sizeof(client_addr);
            int connfd = accept(curFd, reinterpret_cast<struct sockaddr*>(&client_addr), &socklen);
            if (connfd < 0) {
                continue;   // error
            }

            while (true) {
                int32_t err = perform_request(connfd);
                if (err) {
                    break;
                }
            }
            close(connfd);
        }
    }
};