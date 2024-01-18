#include <iostream>
#include <memory>
#include "src/utils/hashtable.hpp"

#include <map>

struct Node {
    int data;
    Node *next;

    ~Node() {
        std::cout << "deleted: " << data << std::endl;
        if (next != nullptr) {
            delete next;
        }
    }
};

int main() {

    return 0;
}