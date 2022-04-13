#pragma once

#ifndef SOCKETLIB_TCPSOCKET_H
#define SOCKETLIB_TCPSOCKET_H

#include "EventService.h"

namespace agpc {

    class TcpSocket {
    public:

        TcpSocket() {}

        void create() {
            if (fd_ != INVALID_FD_VAL) {
                throw std::runtime_error("socket already created");
            }

            fd_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            if (fd_ == INVALID_FD_VAL) {
                std::cout << "unable to create a tcp socket [" << errno << "]" << std::endl;
                throw std::runtime_error("unable to create tcp socket");
            }

            setNonBlocking();
            setNoDelay();
        }

        bool connect(const sockaddr_in &sa) {
            int result = ::connect(fd_, (sockaddr *) &sa, sizeof(sa));
            if (result == 0)
                return true;

            switch (errno) {
                case EWOULDBLOCK:
                case EINPROGRESS:
                case ETIMEDOUT:
                    return false;
                default:
                    std::cout << "socket connect failed [" << errno << "]" << std::endl;
                    throw std::runtime_error("socket connect failed");
            }

        }

        bool accept(TcpSocket &sock) {
            sockaddr_in addr;
            socklen_t len = sizeof(addr);
            int accepted = ::accept(fd_, (sockaddr *) &addr, &len);

            if (accepted != INVALID_FD_VAL) {
                sock.setFD(accepted);
                sock.setNonBlocking();
                return true;
            }

            return false;
        }

        void setReuseAddr(bool reuse) {
            int r = (reuse ? 1 : 0);
            int result = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (const char *) &r, sizeof(r));
            if (result < 0) {
                std::cout << "setReuseAddr failed [" << errno << "]" << std::endl;
                throw std::runtime_error("setReuseAddr failed");
            }
        }

        void bind(const sockaddr_in &sa) {
            int result = ::bind(fd_, (sockaddr *) &sa, sizeof(sa));
            if (result != 0) {
                std::cout << "failed to bind socket " << errno << std::endl;
                throw std::runtime_error("failed to bind socket");
            }
        }

        void listen(int backlog) {
            int result = ::listen(fd_, backlog);

            if (result < 0) {
                std::cout << "failed to listen on tcp socket" << std::endl;
                throw std::runtime_error("failed to listen on tcp socket");
            }
        }

        void setNonBlocking() {
            int flags = ::fcntl(fd_, F_GETFL);
            flags |= O_NONBLOCK;
            if (::fcntl(fd_, F_SETFL, flags) == -1) {
                std::cout << "faild to set socket non blocking [" << errno << "]" << std::endl;
                throw std::runtime_error("failed to set socket non blocking");
            }
        }

        void setRecvBufferSize(int size) {
            int result = ::setsockopt(fd_, SOL_SOCKET, SO_RCVBUF, (const char *) &size, sizeof(size));
            if (result < 0) {
                std::cout << "setRecvBufferSize failed [" << errno << "]" << std::endl;
                throw std::runtime_error("setRecvBufferSize failed");
            }
        }

        void setSendBufferSize(int size) {
            int result = ::setsockopt(fd_, SOL_SOCKET, SO_SNDBUF, (const char *) &size, sizeof(size));
            if (result < 0) {
                std::cout << "setSendBufferSize failed [" << errno << "]" << std::endl;
                throw std::runtime_error("setSendBufferSize failed");
            }
        }

        void setNoDelay() {
            int nd = 1;
            int result = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (const char *) &nd, sizeof(nd));

            if (result < 0) {
                std::cout << "failed to set nodelay sock option" << std::endl;
            }
        }

        ssize_t receive(char *buffer, size_t len, int flags) {
            ssize_t result = ::recv(fd_, (char *) buffer, len, flags);
            if (result > 0)
                return result;

            if (result == 0) {
                return -1; // conenction closed
            }

            switch (errno) {
                case EWOULDBLOCK:
                case ETIMEDOUT:
                    return 0;
                default:
                    // handle these better, not enough time at this moment.
                    return -1;
            }
        }

        ssize_t send(const char *buffer, size_t len, int flags) {
            ssize_t result = ::send(fd_, buffer, len, flags);
            if (result >= 0) {
                return result;
            }

            switch (errno) {
                case EWOULDBLOCK:
                case ETIMEDOUT:
                    return 0;
                default:
                    return -1;
            }
        }

        void close() {
            if (fd_ != INVALID_FD_VAL)
                ::close(fd_);
        }

        int getFD() { return fd_; }

        int setFD(int fd) { fd_ = fd; }

    protected:

        int fd_{INVALID_FD_VAL};

    };
}

#endif //SOCKETLIB_TCPSOCKET_H
