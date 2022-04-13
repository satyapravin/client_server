#include <iostream>
#include <random>
#include "../socklib/EventService.h"
#include "../socklib/TcpSocket.h"
#include "../socklib/ByteBuffer.h"

namespace agpc {

#pragma pack(push, 1)
    struct OutgoingMessage {
        int32_t exchangeNumber_;
        int64_t value_;

        enum {
            LENGTH = 4 + 8
        };
    };

    class Exchange : public EventNode {
    public:

        Exchange(int exch_num, int port)
                : exch_num_(exch_num), port_(port), gen_(rd_()), distr_(1, 1000) {
        }

        void start() {
            addr_.sin_family = AF_INET;
            addr_.sin_addr.s_addr = htonl(INADDR_ANY);
            addr_.sin_port = htons(port_);

            sock_.create();
            sock_.connect(addr_);

            eventService_.registerHandler(sock_.getFD(), this);
            running_ = true;
            eventService_.poll();
        }

        void onRead() override {
            recvBuffer_.compact();

            ssize_t bytes = sock_.receive(recvBuffer_.wPtr(), recvBuffer_.wSize(), 0);

            if (bytes == -1) {
                std::cout << "server disconnected " << sock_.getFD() << std::endl;
                eventService_.removeFD(sock_.getFD());
            }
        }

        void onWrite() override {
            //std::cout << "exchange on write called" << std::endl;
            send();
        }

        bool isWriter() override {
            return true;
        }

        void send() {

            for(int n=0; n<5; ++n) {
                OutgoingMessage omsg;
                omsg.exchangeNumber_ = exch_num_;

                if(curent_count_ < stop_after_max_) {
                    omsg.value_ = distr_(gen_);
                }else{
                    omsg.value_ = 0;
                    stop_ = true;
                }

                curent_count_++;
                sock_.send(reinterpret_cast<char *>(&omsg), omsg.LENGTH, 0);

                if(stop_)
                {
                    eventService_.removeFD(sock_.getFD());
                    sock_.close();
                    eventService_.stop();

                }
            }
        }

     protected:

        std::random_device rd_;
        std::mt19937 gen_;
        std::uniform_int_distribution<> distr_;

        sockaddr_in addr_;
        bool running_{false};
        int port_;
        int exch_num_;
        TcpSocket sock_;
        EventService eventService_;
        ByteBuffer<1024> recvBuffer_;
        int stop_after_max_{50000};
        int curent_count_{0};
        bool stop_{false};
    };
}

using namespace agpc;

int main(int argc, char *argv[]) {

    if (argc != 3) {
        throw std::runtime_error("usage : ./Exchange <id> <port_number> ");
    }

    std::string port_num_str = argv[2];
    std::string exch_num_str = argv[1];

    int port_num = std::stoi(port_num_str);
    int exch_num = std::stoi(exch_num_str);

    Exchange e(exch_num, port_num);
    e.start();

}
