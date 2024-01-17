//
// @brief: A series of exceptions
// @birth: created by Tianyi on 2024/01/08
// @version: V0.0.1
//

#pragma once

#include <cstdint>
#include <cstring>
#include <exception>

class BaseException : std::exception {
    static const int MAX_MSG_LEN = 512;

public:
    char msg_e[MAX_MSG_LEN];

    BaseException(const char* msg) {
        size_t len_msg = strlen(msg);
        assert(len_msg <= MAX_MSG_LEN);
        memcpy(msg_e, msg, len_msg);
    }
};