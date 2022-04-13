#include <iostream>
#include <sstream>
#include <vector>
#include <atomic>
#include <thread>

std::atomic_flag lock = ATOMIC_FLAG_INIT;
std::atomic_int current_id(1);

namespace agpc {

    class printer {
    public:
        printer(const std::string &prn_string, int char_count, int num_print, int id, int thread_count)
                : prn_string_(prn_string), char_count_(char_count), num_print_(num_print), id_(id),
                  thread_count_(thread_count), next_loc_{0}, jump_(0) {

            // keep string ready for each thread
            std::stringstream ss;
            ss << "Thread";
            ss << id_;
            ss << ": ";

            std::string place_holder(char_count, 'X');

            ss << place_holder;
            ss << '\n';
            out_string_ = ss.str();

            prn_string_len_ = prn_string_.length();

            next_loc_ = ((id - 1) * char_count) % prn_string_len_;
            jump_ = char_count * thread_count;

            for (int i = 0; i < char_count_; i++) {
                out_string_[9 + i] = prn_string_[(next_loc_ + i) % prn_string_len_];
            }
        }

        void run() {

            // spin lock. it is my trun. print string and increment atomic to next
            for (int cnt = 0; cnt < num_print_;) {
                bool prep_next = false;

                while (lock.test_and_set(std::memory_order_acquire));// { std::this_thread::yield(); }
                if (current_id == id_) {
                    std::cout << out_string_;
                    ++cnt;
                    prep_next = true;
                    if (current_id == thread_count_) {
                        current_id = 1;
                    } else {
                        current_id++;
                    }
                }
                lock.clear(std::memory_order_release);

                // out of spin lock, do some prep for next time I get my turn
                if (prep_next) {
                    next_loc_ = (next_loc_ + jump_) % prn_string_len_;

                    for (int i = 0; i < char_count_; i++) {
                        out_string_[9 + i] = prn_string_[(next_loc_ + i) % prn_string_len_];

                    }
                }
            }
        }

    protected:
        int jump_;
        int next_loc_;
        std::string prn_string_;
        int char_count_;
        int num_print_;
        std::string out_string_;
        int id_;
        int prn_string_len_;
        int thread_count_;

    };

    class circular_printer {
    public:
        circular_printer(std::string &prn_string, int char_count, int thread_count, int num_print) : num_print_{
                num_print} {

            for (int i = 0; i < thread_count; i++) {
                printers_.emplace_back(&printer::run, printer(prn_string, char_count, num_print, i + 1, thread_count));
            }

            for (auto &x : printers_) {
                x.join();
            }
        }

    private:

        std::vector<std::thread> printers_;
        int num_print_;
    };
}

using namespace agpc;

int main(int argc, char *argv[]) {

    if (argc != 5) {

        throw std::runtime_error("usage : ./CircularPrinter <full_string> <char_count> <thread_count> <num_print>");
    }

    std::string full_string = argv[1];
    std::string char_count_str = argv[2];
    std::string thread_count_str = argv[3];
    std::string num_print_str = argv[4];

    int char_count = std::stoi(char_count_str);
    int thread_count = std::stoi(thread_count_str);
    int num_print = std::stoi(num_print_str);

    circular_printer cp(full_string, char_count, thread_count, num_print);

    return 0;
}
