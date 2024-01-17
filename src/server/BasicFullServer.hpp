//
// @brief: A basic redis server
// @birth: Created by Tianyi on 2024/01/17.
// @version: V0.0.1
//

#pragma once

#include <functional>
#include <map>
#include <vector>
#include <unordered_map>

#include "EventLoopServer.hpp"
#include "../utils/result_status.hpp"

#define USE_STL_MAP
class BasicFullServer : public EventLoopServer {
protected:
#ifdef USE_STL_MAP
    static std::map<std::string, std::string> g_map;
#elif defined(USE_MY_MAP)
    static my_map<std::string, std::string> g_map;
#endif
    static const size_t K_MAX_ARGS = 1024;

    static int32_t parse_req(const uint8_t *data, size_t len, std::vector<std::string> &out) {
        if (len < 4) {
            return -1;
        }

        uint32_t n = 0;
        memcpy(&n, &data[0], 4);
        if (n > K_MAX_ARGS) {
            return -1;
        }

        size_t pos = 4;
        while (n--) {
            if (pos + 4 > len) {
                return -1;
            }
            uint32_t cur_size = 0;
            memcpy(&cur_size, &data[pos], 4);
            if (pos + 4 + cur_size > len) {
                return -1;
            }
            out.emplace_back(reinterpret_cast<const char *>(&data[pos + 4]), cur_size);
            pos += 4 + cur_size;
        }
        if (pos != len) {
            return -1;
        }
        return 0;
    }

    // perform get operation
    //      +-----+-----+
    // cmd: | get | str |
    //      +-----+-----+
    static uint32_t do_get(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
        if (!g_map.count(cmd[1])) {
            return RES_NX;
        }

        std::string &val = g_map[cmd[1]];
        assert(val.size() <= K_MAX_MSG);
        memcpy(res, val.data(), val.size());

        *reslen = static_cast<uint32_t>(val.size());

        return RES_OK;
    }

    static uint32_t do_set(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
        g_map[cmd[1]] = cmd[2];
        return RES_OK;
    }

    static uint32_t do_del(const std::vector<std::string> &cmd, uint8_t *res, uint32_t *reslen) {
        g_map.erase(cmd[1]);
        return RES_OK;
    }

    static bool cmd_is(const std::string &word, const char *cmd) {
        return 0 == strcasecmp(word.c_str(), cmd);
    }

    static int32_t do_request(
            const uint8_t *req, uint32_t reqlen,
            uint32_t *rescode, uint8_t *res, uint32_t *reslen) {
        std::vector<std::string> cmd;
        if (0 != parse_req(req, reqlen, cmd)) {
            msg("bad request");
            return -1;
        }
        printf("client cmd length: %zu, command: ", cmd.size());
        for (const auto& c: cmd) {
            printf("%s ", c.data());
        }
        printf("\n");
        if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
            *rescode = do_get(cmd, res, reslen);
        } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")){
            *rescode = do_set(cmd, res, reslen);
        } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")){
            *rescode = do_del(cmd, res, reslen);
        } else {
            *rescode = RES_ERR;
            const char *msg = "Unknown cmd";
            strcpy(reinterpret_cast<char *>(res), msg);
            *reslen = strlen(msg);
            return 0;
        }

        return 0;
    }

    bool try_one_request(Conn *conn) override{
        // try to parse a request from the buffer
        if (conn->rbuf_size < 4) {
            // not enough data in the buffer. Will retry in the next iteration
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
            // not enough data in the buffer. Will retry in the next iteration
            return false;
        }

        // got one request, generate the response.
        uint32_t rescode = 0;
        uint32_t wlen = 0;
        int32_t err = do_request(
                &conn->rbuf[4], len,
                &rescode, &conn->wbuf[4 + 4], &wlen
        );
        if (err) {
            conn->state = STATE_END;
            return false;
        }
        wlen += 4;
        memcpy(&conn->wbuf[0], &wlen, 4);
        memcpy(&conn->wbuf[4], &rescode, 4);
        conn->wbuf_size = 4 + wlen;

        // remove the request from the buffer.
        // note: frequent memmove is inefficient.
        // note: need better handling for production code.
        size_t remain = conn->rbuf_size - 4 - len;
        if (remain) {
            memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
        }
        conn->rbuf_size = remain;

        // change state
        conn->state = STATE_RES;
        state_res(conn);

        // continue the outer loop if the request was fully processed
        return (conn->state == STATE_REQ);
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
        addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
        int rv = bind(curFd, (const sockaddr *)&addr, sizeof(addr));
        if (rv) {
            die("bind()");
        }

        // listen
        rv = listen(curFd, SOMAXCONN);
        if (rv) {
            die("listen()");
        }

        // a map of all client connections, keyed by fd
        std::vector<Conn *> fd2conn;

        // set the listen fd to nonblocking mode
        fd_set_nb(curFd);

        // the event loop
        std::vector<struct pollfd> poll_args;
        while (true) {
            // prepare the arguments of the poll()
            poll_args.clear();
            // for convenience, the listening fd is put in the first position
            struct pollfd pfd = {curFd, POLLIN, 0};
            poll_args.push_back(pfd);
            // connection fds
            for (Conn *conn : fd2conn) {
                if (!conn) {
                    continue;
                }
                struct pollfd pfd = {};
                pfd.fd = conn->fd;
                pfd.events = (conn->state == STATE_REQ) ? POLLIN : POLLOUT;
                pfd.events = pfd.events | POLLERR;
                poll_args.push_back(pfd);
            }

            // poll for active fds
            // the timeout argument doesn't matter here
            int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
            if (rv < 0) {
                die("poll");
            }

            // process active connections
            for (size_t i = 1; i < poll_args.size(); ++i) {
                if (poll_args[i].revents) {
                    Conn *conn = fd2conn[poll_args[i].fd];
                    connection_io(conn);
                    if (conn->state == STATE_END) {
                        // client closed normally, or something bad happened.
                        // destroy this connection
                        fd2conn[conn->fd] = NULL;
                        (void)close(conn->fd);
                        free(conn);
                    }
                }
            }

            // try to accept a new connection if the listening fd is active
            if (poll_args[0].revents) {
                (void)accept_new_conn(fd2conn, curFd);
            }
        }

        return 0;
    }
};

std::map<std::string, std::string> BasicFullServer::g_map;

