//
// @brief: A basic redis client
// @birth: Created by Tianyi on 2024/01/17.
// @version: V0.0.1
//

#pragma once

#include <map>
#include <vector>

#include "EventLoopClient.hpp"

class BasicFullClient : public EventLoopClient {

    // send request
    //          +-----+-------+------+------+------+------+
    //  cmd:    | len | nargs | len1 | arg1 | len2 | arg2 | ...
    //          +-----+-------+------+------+------+------+
    //             4      4      4     len1     4    len2
    static int32_t send_req(int fd, const std::vector<std::string> &cmd) {
        uint32_t len = 4;
        for (const auto &s: cmd) {
            len += 4 + s.size();
        }
        if (len > K_MAX_MSG) {
            return -1;
        }

        char wbuf[4 + K_MAX_MSG];
        memcpy(&wbuf[0], &len, 4);
        uint32_t n = cmd.size();
        memcpy(&wbuf[4], &n, 4);
        size_t cur_pos = 8;

        for (const auto &s: cmd) {
            uint32_t cur_size = s.size();
            memcpy(&wbuf[cur_pos], &cur_size, 4);
            memcpy(&wbuf[cur_pos + 4], s.data(), s.size());
            cur_pos += 4 + s.size();
        }
        return write_all(fd, wbuf, 4 + len);
    }

    static int32_t read_res(int fd) {
        // 4 bytes header
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

        uint32_t len = 0;
        memcpy(&len, rbuf, 4);  // assume little endian
        if (len > K_MAX_MSG) {
            msg("too long");
            return -1;
        }

        // reply body
        err = read_full(fd, &rbuf[4], len);
        if (err) {
            msg("read() error");
            return err;
        }

        // print the result
        uint32_t rescode = 0;
        if (len < 4) {
            msg("bad response");
            return -1;
        }
        memcpy(&rescode, &rbuf[4], 4);
        printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
        return 0;
    }

public:
    int work(int argc, char **argv) {
        curFd = socket(AF_INET, SOCK_STREAM, 0);
        if (curFd < 0) {
            die("socket");
        }

        struct sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_port = ntohs(1234);
        addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
        int rv = connect(curFd, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(addr));
        if (rv) {
            die("connect");
        }

        std::vector<std::string> cmd;
        for (int i = 1; i < argc; ++i) {
            cmd.emplace_back(argv[i]);
        }

        int32_t err = send_req(curFd, cmd);
        if (err) {
            close(curFd);
            return 0;
        }

        err = read_res(curFd);
        if (err) {
            close(curFd);
            return 0;
        }

        close(curFd);
        return 0;
    }
};

