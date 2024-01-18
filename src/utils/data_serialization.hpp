//
// @brief: data serialization utilities
// @birth: created by Tianyi on 2024/01/19
// @version: V0.0.1
//

#ifndef DATA_SERIALIZATION
#define DATA_SERIALIZATION

enum SER_ERR {
    ERR_UNKNOWN = 1,
    ERR_2BIG = 2,
};

enum SER_RESULT {
    SER_NIL = 0,
    SER_ERR = 1,
    SER_STR = 2,
    SER_INT = 3,
    SER_ARR = 4,
};



#endif