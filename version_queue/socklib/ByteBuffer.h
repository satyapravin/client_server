#pragma once

#ifndef SOCKETLIB_BYTEBUFFER_H
#define SOCKETLIB_BYTEBUFFER_H

#include <cstdint>
#include <cstddef>
#include <cstring>

namespace agpc {

    template<int32_t SIZE = 1024>
    class ByteBuffer {
    public:

        ByteBuffer() : rPos_(buf_), wPos_(buf_), end_(buf_ + SIZE) {}

        const char *begin() const { return buf_; }

        const char *rPtr() const { return rPos_; }

        const char *rEnd() const { return wPos_; }

        char *wPtr() { return wPos_; }

        const char *wEnd() const { return end_; }

        size_t rSize() const { return wPos_ - rPos_; }

        size_t wSize() const { return end_ - wPos_; }

        size_t capacity() const { return end_ - buf_; }

        size_t rPosition() const { return rPos_ - buf_; }

        size_t wPosition() const { return wPos_ - buf_; }

        void rAdvance(const size_t length) {
            rPos_ += length;
        }

        void wAdvance(const size_t length) {
            wPos_ += length;
        }

        void clear() { rPos_ = wPos_ = buf_; }

        void rPosition(size_t position) {
            rPos_ = buf_ + position;
        }

        void wPosition(size_t position) {
            wPos_ = buf_ + position;
        }

        template<typename T>
        void read(T &val) {
            val = *reinterpret_cast<T const *>(rPos_);
            rPos_ += sizeof(T);
        }

        void compact() {
            if (rPos_ != buf_) {
                size_t n = rSize();
                if (n) {
                    memmove(buf_, rPos_, n);
                    rPos_ = buf_;
                    wPos_ = buf_ + n;
                } else {
                    clear();
                }
            }
        }

        bool put(const char *bytes, size_t length) {
            if (wSize() < length) {
                compact();
                if (wSize() < length)
                    return false;
            }
            memcpy(wPos_, bytes, length);
            wPos_ += length;
            return true;
        }

        template<typename T>
        bool putBytes(T const &incoming) {
            return put(reinterpret_cast<const char *>(&incoming), sizeof(incoming));
        }

    protected:

        char buf_[SIZE];
        char *rPos_;
        char *wPos_;
        char *end_;

    };
}


#endif //SOCKETLIB_BYTEBUFFER_H

