#include <cstdint>
#include "../socklib/EventService.h"
#include "../socklib/ClientConnection.h"
#include "max_heap.h"

namespace agpc {

    class SortServer : public EventNode {
    public:
        typedef ClientConnectionT<SortServer> ClientConnection;

        SortServer(int port, int16_t backlog = 5) : port_(port), backlog_(backlog) {
            std::memset(&addr_, '\0', sizeof(addr_));
        }

        void start() {
            bind();
            listen();

            eventService_.poll();
        }

        void bind() {
            try {
                sock_.create();
                sock_.setReuseAddr(true);

                addr_.sin_family = AF_INET;
                addr_.sin_addr.s_addr = htonl(INADDR_ANY);
                addr_.sin_port = htons(port_);
                sock_.bind(addr_);
            }
            catch (const std::exception &e) {
                std::cout << "exception while creating socket " << e.what() << std::endl;
                eventService_.removeFD(sock_.getFD());
                sock_.close();
                throw;
            }
        }

        void listen() {
            try {
                sock_.listen(backlog_);
                eventService_.registerHandler(sock_.getFD(), this);
                running_ = true;
            }
            catch (const std::exception &e) {
                std::cout << "exception while listening for incoming connections " << e.what() << std::endl;
                sock_.close();
                throw;
            }
        }

        void stop() {
            if (running_) {
                eventService_.removeFD(sock_.getFD());
                running_ = false;
            }

            sock_.close();
            eventService_.stop();
        }

        void onRead() override {
            TcpSocket client_socket;
            while (sock_.accept(client_socket)) {
                ClientConnection *c = new ClientConnection(eventService_, client_socket, this);
                connections_.push_back(c);
            }
            // do more error checking/handling if have time.
        }

        void onMsg(int32_t exch, int64_t value, ClientConnection *conn) {
            if (value == 0) {
                flush();
                conn->setStopped();
                check_connected_clients();
            } else {
                pq_.enqueue(value);
            }
        }

        void flush() {
            max_heap<int64_t> temp(pq_);
            while (!temp.empty()) {
                std::cout << temp.dequeue() << " ";
            }
            std::cout << std::endl;

        }

        bool isReader() override { return true; }

    protected:

        void check_connected_clients() {
            bool should_stop = true;

            for (ClientConnection *connection : connections_) {
                if (!connection->isStopped()) {
                    should_stop = false;
                    break;
                }
            }

            if (should_stop) {
                std::cout << "all clients finished sending data (received 0 from all), exiting" << std::endl;
                this->stop();
            }
        }

        max_heap<int64_t> pq_;
        sockaddr_in addr_;
        EventService eventService_;
        TcpSocket sock_;
        int16_t backlog_;
        int port_;
        bool running_{false};
        std::vector<ClientConnection *> connections_;

    };
}

using namespace agpc;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        throw std::runtime_error("usage : ./SortServer <port_number>");
    }

    std::string port_num_str = argv[1];
    int port_num = std::stoi(port_num_str);

    SortServer ss(port_num);
    ss.start();
}