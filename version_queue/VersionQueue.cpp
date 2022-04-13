#include <iostream>
#include <list>
#include <type_traits>
#include <iterator>
#include <algorithm>
#include <cassert>
#include <unordered_map>

namespace agpc {

    template<class T, class A = std::allocator<T> >
    class version_queue final {

    public:

        typedef typename A::value_type value_type;
        typedef typename A::size_type size_type;
        typedef typename std::list<T, A>::iterator iterator;

        version_queue() : currentVersion_(0) {
            versionIters_[currentVersion_] = std::make_pair(storage_.begin(), storage_.end());
        }

        iterator begin(int32_t v) {
            if (v > currentVersion_ || v == 0) {
                return storage_.end();
            }
            std::pair<iterator, iterator> &iterPair = versionIters_[v];
            return iterPair.first;
        }

        iterator end(int32_t v) {
            if (v > currentVersion_ || v == 0) {
                return storage_.end();
            }
            std::pair<iterator, iterator> &iterPair = versionIters_[v];
            iterator iter = iterPair.second;

            if (iter != storage_.end()) {
                iter++;
            }
            return iter;
        }

        size_type size(int32_t version_no) {
            size_type sz = 0;
            auto iterBegin = this->begin(version_no);
            auto iterEnd = this->end(version_no);

            while (iterBegin != iterEnd) {
                sz++;
                iterBegin++;
            }

            return sz;
        }

        bool empty(int32_t v) { return this->begin(v) == this->end(v); }

        int32_t current_version() const { return currentVersion_; }

        void enqueue(const value_type &value) {
            std::pair<iterator, iterator> &iterPair = versionIters_[currentVersion_];

            if (iterPair.first == storage_.end() && iterPair.second == storage_.end()) {
                currentVersion_++;
                storage_.push_back(value);
                iterator i = storage_.end();
                --i;
                versionIters_[currentVersion_] = std::make_pair(i, i);
            } else {

                currentVersion_++;
                storage_.push_back(value);
                iterator i = storage_.end();
                versionIters_[currentVersion_] = std::make_pair(iterPair.first, --i);
            }
        }

        void enqueue(const value_type &&value) {
            std::pair<iterator, iterator> &iterPair = versionIters_[currentVersion_];
            if (iterPair.first == storage_.end() && iterPair.second == storage_.end()) {
                // first insert
                currentVersion_++;
                storage_.push_back(value);
                iterator i = storage_.end();
                --i;
                versionIters_[currentVersion_] = std::make_pair(i, i);
            } else {

                currentVersion_++;
                storage_.push_back(value);
                iterator i = storage_.end();
                versionIters_[currentVersion_] = std::make_pair(iterPair.first, --i);
            }
        }

        // problem says that return top element in dequeue function
        // I say its not a good idea, empty queue will have issues with that style
        // unless you want to do some type unsafe void* stuff etc.
        // ideally STL stle pop top idiom should be used, where user checks for empty queue.
        bool dequeue(value_type &val) {
            std::pair<iterator, iterator> &iterPair = versionIters_[currentVersion_];
            bool toreturn = true;
            if (iterPair.first == iterPair.second) {
                toreturn = false;
                val = *iterPair.first;
            }
            currentVersion_++;
            if (iterPair.first == iterPair.second) {
                versionIters_[currentVersion_] = std::make_pair(storage_.end(), storage_.end());
            } else {
                versionIters_[currentVersion_] = std::make_pair(++iterPair.first, iterPair.second);
            }
            return toreturn;
        }

        void print(int32_t version_no) {
            auto iterBegin = this->begin(version_no);
            auto iterEnd = this->end(version_no);

            while (iterBegin != iterEnd) {
                std::cout << *iterBegin << " ";
                iterBegin++;
            }
            std::cout << std::endl;
        }

    private:

        std::list<T, A> storage_;
        int currentVersion_{0};
        std::unordered_map<int, std::pair<iterator, iterator>> versionIters_;
    };
}


using namespace agpc;

template<typename VQ>
void print_vq(VQ &vq, int version_no) {
    auto iterBegin = vq.begin(version_no);
    auto iterEnd = vq.end(version_no);

    while (iterBegin != iterEnd) {
        std::cout << *iterBegin << " ";
        iterBegin++;
    }
    std::cout << std::endl;
}

void test_basic() {
    version_queue<int> vq;
    version_queue<int>::size_type sz;
    version_queue<int>::value_type val;

    vq.enqueue(91);
    sz = vq.size(1);
    assert(sz == 1);
    vq.dequeue(val);
    assert(val == 91);
    sz = vq.size(2);
    assert(sz == 0);
    vq.enqueue(2);
    sz = vq.size(3);
    assert(sz == 1);

    print_vq(vq, 1);
    print_vq(vq, 2);
    print_vq(vq, 3);
}

void test_deque_empty() {
    version_queue<int> vq;
    version_queue<int>::size_type sz;

    version_queue<int>::value_type val;
    bool ret = vq.dequeue(val);
    sz = vq.size(1);
    assert(sz == 0);
    assert(ret == false);
}

void test_hist_versions() {
    version_queue<int> vq;
    version_queue<int>::size_type sz;
    version_queue<int>::value_type val;

    // version 1, 91
    vq.enqueue(91);
    sz = vq.size(1);
    assert(sz == 1);

    // version 2 , <empty>
    vq.dequeue(val);
    sz = vq.size(2);
    assert(sz == 0);
    assert(val == 91);

    // version 3 , 2
    vq.enqueue(2);
    sz = vq.size(3);
    assert(sz == 1);

    print_vq(vq, 1);
    print_vq(vq, 2);
    print_vq(vq, 3);

    // version 4, empty
    vq.dequeue(val);
    print_vq(vq, 4);

    // version 5, empty
    bool ret = vq.dequeue(val);
    print_vq(vq, 5);
    assert(ret == false);

    // version 6, 55
    vq.enqueue(55);
    print_vq(vq, 6);

    // version 7, 55, 44
    vq.enqueue(44);
    print_vq(vq, 7);

    sz = vq.size(6);
    assert(sz == 1);

    sz = vq.size(4);
    assert(sz == 0);
}

int main() {

    test_basic();
    test_deque_empty();
    test_hist_versions();

    return 0;
}
