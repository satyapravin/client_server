#ifndef SOCKETLIB_CLIENTCONNECTION_H
#define SOCKETLIB_CLIENTCONNECTION_H

#include "EventService.h"
#include "TcpSocket.h"
#include "ByteBuffer.h"

#define bswap64(y) (((uint64_t)ntohl(y)) << 32 | ntohl(y>>32))

namespace agpc {

#pragma pack(push, 1)

    struct IncomingMessage {
        int32_t exchangeNumber_;
        int64_t value_;

        enum {
            LENGTH = 4 + 8
        };

        int32_t getExchangeNTOH() const { return ntohl(exchangeNumber_); }

        int64_t getValueNTOH() const { return bswap64(value_); }

        int32_t getExchange() const { return exchangeNumber_; }

        int64_t getValue() const { return value_; }

    };

#pragma pack(pop)

    template<typename HANDLER>
    class ClientConnectionT : public EventNode {
    public:
        ClientConnectionT(EventService &eventService, TcpSocket &tcpSocket, HANDLER *handler)
                : eventService_(eventService), sock_(tcpSocket), handler_(handler) {
            eventService_.registerHandler(sock_.getFD(), this);
        }

        void onRead() override {
            recvBuffer_.compact();

            ssize_t bytes = sock_.receive(recvBuffer_.wPtr(), recvBuffer_.wSize(), 0);

            if (bytes == -1) {
                eventService_.removeFD(sock_.getFD());
                return;
            }

            recvBuffer_.wAdvance(bytes);

            bool sent_something = false;

            while (recvBuffer_.rSize() >= 12) {
                IncomingMessage msg;
                recvBuffer_.read(msg);
                handler_->onMsg(msg.getExchange(), msg.getValue(), this);
                sent_something = true;
            }

            if(sent_something){
                handler_->flush();
            }
        }

        void onWrite() override {
            std::cout << "on Write called on socket connection." << std::endl;
        }

        bool isReader() override { return true; }

        bool isStopped() const { return stopped_; }

        void setStopped() { stopped_ = true; }


    protected:

        bool stopped_{false};
        TcpSocket sock_;
        EventService &eventService_;
        HANDLER *handler_;
        ByteBuffer<1024> recvBuffer_;
    };


}
#endif //SOCKETLIB_CLIENTCONNECTION_H
