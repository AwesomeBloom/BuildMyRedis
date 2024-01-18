//
// @brief: A basic Implementation of event loop
// @birth: created by Tianyi on 2024/01/09
// @version: V0.0.1
//

#pragma once

#include <cstdint>

#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

enum EVENT_STATE {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
};

const int K_MAX_MSG = 4096;

struct Conn {
    int fd = -1;
    EVENT_STATE state = STATE_REQ;

    // buffer for reading
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + K_MAX_MSG]{};

    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + K_MAX_MSG]{};
};
