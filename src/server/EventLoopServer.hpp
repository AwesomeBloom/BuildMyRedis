//
// @brief: A Server with Event Loop Implementation
// @birth: created by Tianyi on 2024/01/16
// @version: V0.0.1
//

#pragma once

#include <poll.h>

#include <vector>

#include "BaseServer.hpp"
#include "../utils/EventLoop.hpp"

class EventLoopServer : public BaseServer {
protected:
    // function which can set the listen fd to nonblocking mode
    void fd_set_nb(int fd) {
        errno = 0;
        int flags = fcntl(fd, F_GETFL, 0);
        if (errno) {
            die("fcntl error");
            return ;
        }

        flags |= O_NONBLOCK;

        errno = 0;
        fcntl(fd, F_SETFL, flags);
        if (errno) {
            die("fcntl error");
        }
    }

    void conn_put(std::vector<Conn *> &fd2conn, Conn *conn) {
        if (fd2conn.size() <= static_cast<size_t>(conn->fd)) {
            fd2conn.resize(conn->fd + 1);
        }
        fd2conn[conn->fd] = conn;
    }

    int32_t accept_new_conn(std::vector<Conn*> &fd2conn, int fd) {
        // accept in <socket.h>
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, reinterpret_cast<sockaddr*>(&client_addr), &socklen);
        if (connfd < 0) {
            msg("accept error");
            return -1;
        }

        // set the new connection fd to nonblocking mode
        fd_set_nb(connfd);

        auto *conn = new Conn;
        if (!conn) {
            close(connfd);
            msg("create conn error");
            return -1;
        }

        // initialize conn
        conn->fd = connfd;
        conn->state = STATE_REQ;
        conn->rbuf_size = 0;
        conn->wbuf_size = 0;
        conn->wbuf_sent = 0;

        conn_put(fd2conn, conn);
        return 0;
    }

    void state_req(Conn *conn) {
        while (try_fill_buffer(conn)) {}
    }

    virtual bool try_one_request(Conn *conn) {
        //try to parse a request from the buffer
        if (conn->rbuf_size < 4) {
            // not enough data in buffer, retry in next iteration
            return false;
        }

        uint32_t len = 0;
        memcpy(&len, &conn->rbuf[0], 4);

        if (len > K_MAX_MSG) {
            msg("message is too long");
            conn->state = STATE_END;
            return false;
        }

        if (4 + len > conn->rbuf_size) {
            // not enough data in buffer, retry in next iteration
            return false;
        }

        // got one request, print it (or do something else)
        printf("client says: %.*s\n", len, &conn->rbuf[4]);

        // generating reply (server echo)
        memcpy(&conn->wbuf[0], &len, 4);
        memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
        conn->wbuf_size = 4 + len;

        // remove the request from the buffer (might contain address overlapping!)
        size_t remain = conn->rbuf_size - 4 - len;
        if (remain) {
            memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
        }
        conn->rbuf_size = remain;

        // change state
        conn->state = STATE_RES;
        state_res(conn);

        // continue the outer loop if th request was fully processed
        return (conn->state == STATE_REQ);
    }

    bool try_fill_buffer(Conn *conn) {
        // try to fill the buffer
        assert(conn->rbuf_size < sizeof(conn->rbuf));
        ssize_t rv = 0;
        do {
            size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
            rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
        } while (rv < 0 && errno == EINTR);

        if (rv < 0 && errno == EAGAIN) {
            // got EAGAIN, and stop. (continuous reading but there's no data)
            return false;
        }

        if (rv < 0) {
            msg("read() error");
            conn->state = STATE_END;
            return false;
        } else if (rv == 0) {
            if (conn->rbuf_size > 0) {
                msg("unexpected EOF");
            } else {
                msg("EOF");
            }
            conn->state = STATE_END;
            return false;
        }

        conn->rbuf_size += static_cast<size_t>(rv);
        assert(conn->rbuf_size <= sizeof(conn->rbuf));

        while (try_one_request(conn)) {};
        return (conn->state == STATE_REQ);

    }

    bool try_flush_buffer(Conn *conn) {
        ssize_t rv = 0;
        do {
            size_t remain = conn->wbuf_size - conn->wbuf_sent;
            rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
        } while (rv < 0 && errno == EINTR);

        if (rv < 0 && errno == EAGAIN) {
            // got eagain, stop. (continuous writing but there's no data)
            return false;
        }

        if (rv < 0) {
            msg("write() error");
            conn->state = STATE_END;
        }

        conn->wbuf_sent += static_cast<size_t>(rv);
        assert(conn->wbuf_sent <= conn->wbuf_size);

        if (conn->wbuf_sent == conn->wbuf_size) {
            // fully sent, change state;
            conn->state = STATE_REQ;
            conn->wbuf_sent = 0;
            conn->wbuf_size = 0;
            return false;
        }
        // still got some date in wbuf
        return true;
    }

    void connection_io(Conn *conn) {
        if (conn->state == STATE_REQ) {
            state_req(conn);
        } else if (conn->state == STATE_RES) {
            state_res(conn);
        } else {
            // not expected
            assert(false);
        }

    }

    static const size_t K_MAX_MSG = 4096;

    void state_res(Conn *conn) {
        while (try_flush_buffer(conn)) {}
    }

public:
    int work() override {
        curFd = socket(AF_INET, SOCK_STREAM, 0);
        if (curFd < 0) {
            die("socket()");
        }

        int val = 1;
        setsockopt(curFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

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
            die("listen()");
        }

        // a map of all client connections, keyed by fd
        // TODO(Tianyi): change it to hashtable
        std::vector<Conn *> fd2conn;

        fd_set_nb(curFd);

        std::vector<struct pollfd> poll_args;

        while (true) {
            // prepare the arguments of the poll()
            poll_args.clear();

            // for convenience, the listening fd is put in the first position
            struct pollfd pfd = {curFd, POLLIN, 0};
            poll_args.push_back(pfd);

            // connection fds
            for (auto conn: fd2conn) {
                if (!conn) {
                    continue;
                }
                pfd = {};
                pfd.fd = conn->fd;
                pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
                pfd.events |= POLLERR;
                poll_args.push_back(pfd);
            }

            // poll for active fds
            // the timeout argument doesn't matter here
            // TODO(Tianyi): Change it into epoll
            int rv = poll(poll_args.data(), static_cast<nfds_t>(poll_args.size()), 1000);
            if (rv < 0) {
                die("poll");
            }

            // process active connections
            // from index 1 (index 0 is the listening fd)
            auto len_poll_args = poll_args.size();
            for (auto i = 1; i < len_poll_args; ++i) {
                if (poll_args[i].revents) {
                    auto conn = fd2conn[poll_args[i].fd];
                    connection_io(conn);
                    if (conn->state == STATE_END) {
                        // client closed (normally or sth bad happened)
                        // close this connection
                        fd2conn[conn->fd] = nullptr;
                        close(conn->fd);
                        delete(conn);
                    }
                }
            }

            // try to accept a new connection if the listening fd is active
            if (poll_args[0].revents) {
                accept_new_conn(fd2conn, curFd);
            }
        }

        return 0;
    }

};