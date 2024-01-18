//
// @brief: A basic Implementation of hashtable
// @birth: created by Tianyi on 2024/01/18
// @version: V0.0.1
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>

#include <functional>

// for intrusive implementation
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );})

template<class _Key, class _Val, class _HashRes=int>
struct Entry {
    struct HNode<_HashRes> node;
    _Key key;
    _Val val;
};

template<class _Key, class _Val, class _HashRes=int>
static bool entry_eq(HNode<_HashRes> *lhs, HNode<_HashRes> *rhs) {
    Entry<_Key, _Val> *le = container_of(lhs, (Entry<_Key, _Val>), node);
    Entry<_Key, _Val> *re = container_of(rhs, (Entry<_Key, _Val>), node);

    return lhs->code_ == rhs->code_ && le->key == re->key;
}

// uint8(stream) hash function
static uint64_t char_arr_hash(const uint8_t *data, size_t len) {
    uint32_t hash_base = 0x811C9DC5;
        for (size_t i = 0; i < len; i++) {
        hash_base = (hash_base + data[i]) * 0x01000193;
    }
    return hash_base;
}

// implementation of hashtable node
template <class T>
struct HNode {
    T code_;
    HNode* next_;

    HNode(const T &code, HNode* next=nullptr) : code_(code), next_(next) {}

    ~HNode() = default;
};

// implementation of hashtable
template <class T>
struct HTable
{
    HNode<T> **tab_ = nullptr;
    size_t mask_ = 0;
    size_t size_ = 0;

    HTable(size_t n=32) {
        // assert n is a power of 2;
        assert(n > 0 && ((n - 1) & n) == 0);
        tab_ = new HNode<T>* [n];
        mask_ = n - 1;
        size_ = 0;
    }

    ~HTable() {
        if (tab_ != nullptr) {
            for (int i = 0; i <= mask_; ++i) {
                delete tab_[i];        
            }
            delete tab_;
        }
    }

    void resize(size_t n=32) {
        if (tab_ != nullptr) {
            for (int i = 0; i <= mask_; ++i) {
                delete tab_[i];        
            }
            delete tab_;
        }
        // assert n is a power of 2;
        assert(n > 0 && ((n - 1) & n) == 0);
        tab_ = new HNode<T>* [n];
        mask_ = n - 1;
        size_ = 0;
    }

    void insert(HNode<T> *node) {
        size_t pos = node->code_ & mask_;
        HNode<T> *next = tab_[pos];
        node->next_ = next;
        tab_[pos] = node;
        ++size_;
    }

    // hashtable look up subroutine.
    // Pay attention to the return value. It returns the address of
    // the parent pointer that owns the target node,
    // which can be used to delete the target node.
    HNode<T>** find(HNode<T> *key, std::function<bool(HNode<T>*, HNode<T>*)> cmp) {
        if (tab_ == nullptr) {
            return nullptr;
        }

        size_t pos = key->code_ & mask_;
        HNode<T> **from = &tab_[pos];
        while (*from) {
            if (cmp(*from, key)) {
                return from;
            }
            from = &(*from)->next_;
        }
        return nullptr;
    }

    // remove a node from the chain
    HNode<T>* detach(HNode<T> **from) {
        HNode<T> *node = *from;
        *from = (*from)->next_;
        --size_;
        return node;
    }


};

// implementation of hashmap
template <class T>
class MyHashMap {
    HTable<T> ht1_;
    HTable<T> ht2_;
    size_t resizing_pos_ = 0;

    // max work number per resize
    static const size_t K_RESIZING_WORK = 128;
    static const size_t K_MAX_LOAD_FACTOR = 8;
    
    // moves some keys to the new table
    void help_resize() {

        if (ht2_.tab_ == nullptr) {
            return ;
        }

        size_t nwork = 0;
        while (nwork < K_RESIZING_WORK && ht2_.size_ > 0) {
            HNode<T> **from = &ht2_.tab_[resizing_pos_];
            if (*from == nullptr) {
                ++resizing_pos_;
                continue;
            }

            ht1_.insert(ht1_.detach(from));
            ++nwork;
        }

        if (ht2_.size_ == 0) {
            // all done
            ht2_.resize();
        }
    }

    void resize() {
        assert(ht2_.size_ == 0);
        // swap
        ht2_ = ht1_;
        ht1_.resize((ht1.mask_ + 1) << 1);
        resizing_pos_ = 0;
    }

public:
    MyHashMap() {}
    MyHashMap(size_t n) {}

    ~MyHashMap() {

    }

    HNode<T>* find(HNode<T>* key, std::function<bool(HNode<T>*, HNode<T>*)> cmp) {
        help_resize();
        HNode<T> **from = ht1_.find(key, cmp);
        if (!from) {
            from = ht2_.find(key, cmp);
        }
        return from ? *from : nullptr;
    }

    HNode<T>* pop(HNode<T>* key, std::function<bool(HNode<T>*, HNode<T>*)> cmp) {
        help_resize();
        HNode<T> **from = ht1_.find(key, cmp);
        if (from) {
            return ht1.detach(from);
        } else {
            from = ht2_.find(key, cmp);
            if (from) {
                return ht2.detach(from);
            }
        }
        return nullptr;
    }

};

