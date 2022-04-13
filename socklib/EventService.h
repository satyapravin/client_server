#pragma once

#ifndef SOCKETLIB_EVENTSERVICE_H
#define SOCKETLIB_EVENTSERVICE_H

#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <vector>
#include "SockCommon.h"

namespace agpc {

    class EventNode {
    public:

        virtual void onRead() {}

        virtual void onWrite() {}

        virtual bool isReader() { return false; }

        virtual bool isWriter() { return false; }
    };

    class EventService {
    public:

        EventService() : epfd_(INVALID_FD_VAL) {
            epfd_ = epoll_create1(0);
            if (epfd_ < 0) {
                std::cout << "epoll create failed [" << errno << "]" << std::endl;
                throw std::runtime_error("epoll create failed");
            }
        }

        ~EventService() {
            if (epfd_ != INVALID_FD_VAL) {
                close(epfd_);
                epfd_ = INVALID_FD_VAL;
            }
        }

        void stop() { stop_ = true; }

        bool poll() {
            epoll_event eevents[64];
            while (!stop_) {
                int nfds = epoll_wait(epfd_, eevents, ARRAY_SIZE(eevents), -1);
                if (nfds < 0) {
                    std::cout << "epoll returned but no events" << std::endl;
                    continue;
                }

                for (int i = 0; i < nfds; i++) {
                    auto e = eevents[i];
                    EventNode *handler = static_cast<EventNode *>(e.data.ptr);

                    if (e.events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                        if (handler) {
                            handler->onRead();
                        }
                    }
                    if (e.events & (EPOLLOUT | EPOLLERR)) {
                        if (handler) {
                            handler->onWrite();
                        }
                    }
                }
            }
        }

        void registerHandler(int fd, EventNode *handler) {
            epoll_event eevent;
            eevent.data.fd = fd;

            eevent.events = (EPOLLRDHUP | EPOLLPRI |
                             (handler->isReader() ? (int) EPOLLIN : 0) |
                             (handler->isWriter() ? (int) EPOLLOUT : 0)
            );

            eevent.data.ptr = handler;

            if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &eevent) < 0) {
                std::cout << "epoll add failed" << std::endl;
            }
        }

        void removeFD(int fd) {
            if (fd != INVALID_FD_VAL) {
                epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, 0);
            }
        }


    protected:

        int epfd_;
        bool stop_{false};
    };
}


#endif //SOCKETLIB_EVENTSERVICE_H
