// 
// @brief: Basic server
// @birth: created by Tianyi on 2024/01/08
// @version: v0.0.1
// 

#pragma once

#include "BaseServer.hpp"

// A basic server
class BasicServer : public BaseServer {
    
    // react to client
    void do_something(int connfd) {
        char rbuf[BUF_SIZE];
        ssize_t n = read(connfd, const_cast<char*>(rbuf), sizeof(rbuf) - 1);
        if (n < 0) {
            msg("read() error");
            return ;
        }
        
        fprintf(stdout, "client says: %s\n", rbuf);
        
        char wbuf[BUF_SIZE] = "world";
        write(connfd, wbuf, sizeof(wbuf) - 1);
    }

public:
    // main work function
    int work() override {
        curFd = socket(AF_INET, SOCK_STREAM, 0);
        if (curFd < 0) {
            die("socket()");
        } 

        // socket options
        bool bReuseaddr = true;
        setsockopt(curFd, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr, sizeof(bReuseaddr));

        // bind
        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;        
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(0);
        int rv = bind(curFd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
        if (rv) {
            die("bind()");
        }

        // listen
        rv = listen(curFd, SOMAXCONN);
        if (rv) {
            die("listen");
        }
        while (true) {
            // accept
            struct sockaddr_in client_addr = {};
            socklen_t socklen = sizeof(client_addr);
            int connfd = accept(curFd, reinterpret_cast<struct sockaddr*>(&client_addr), &socklen);
            if (connfd < 0) {
                msg("accept() failed");
                continue ;
            }

            do_something(connfd);
            close(connfd);
        }

    }
};