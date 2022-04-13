#ifndef SORTSERVER_MAX_HEAP_H
#define SORTSERVER_MAX_HEAP_H

#include <iostream>
#include <vector>

namespace agpc {

    template <typename T>
    class max_heap {

    public:
        max_heap() {}

        max_heap(std::vector<T> &items) {
            storage_ = items;
            heapify();
        }

        max_heap(const max_heap<T> &rhs) {
            this->storage_ = rhs.storage_;
            heapify();
        }

        void enqueue(T item) {
            storage_.push_back(item);
            shift_left(0, storage_.size() - 1);
            return;
        }

        void print() {
            for (int i = 0; i < storage_.size(); ++i) {
                std::cout << storage_[i] << "   ";
            }
            std::cout << std::endl;
        }

        int dequeue() {
            int last = storage_.size() - 1;
            int tmp = storage_[0];
            storage_[0] = storage_[last];
            storage_[last] = tmp;
            storage_.pop_back();
            shift_right(0, last - 1);
            return tmp;
        }

        bool empty() {
            return storage_.size() == 0;
        }

    protected:

        std::vector<T> storage_;

        void shift_left(int low, int high) {

            int child_index = high;
            while (child_index > low) {

                int parent_index = (child_index - 1) / 2;
                if (storage_[child_index] > storage_[parent_index]) {
                    int tmp = storage_[child_index];
                    storage_[child_index] = storage_[parent_index];
                    storage_[parent_index] = tmp;
                    child_index = parent_index;
                } else {
                    break;
                }
            }
            return;
        }

        void shift_right(int low, int high) {
            int root = low;
            while ((root * 2) + 1 <= high) {
                int left_child = (root * 2) + 1;
                int right_child = left_child + 1;
                int swap_inddx = root;
                if (storage_[swap_inddx] < storage_[left_child]) {
                    swap_inddx = left_child;
                }
                if ((right_child <= high) && (storage_[swap_inddx] < storage_[right_child])) {
                    swap_inddx = right_child;
                }
                if (swap_inddx != root) {
                    int tmp = storage_[root];
                    storage_[root] = storage_[swap_inddx];
                    storage_[swap_inddx] = tmp;
                    root = swap_inddx;
                } else {
                    break;
                }
            }
            return;
        }

        void heapify() {
            int size = storage_.size();
            int mid_index = (size - 2) / 2;
            while (mid_index >= 0) {
                shift_right(mid_index, size - 1);
                --mid_index;
            }
            return;
        }
    };
}

#endif //SORTSERVER_MAX_HEAP_H
