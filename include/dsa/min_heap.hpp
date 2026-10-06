#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace codevault::dsa {

/**
 * @brief Generic Min-Heap (Priority Queue) custom implementation.
 *
 * Maintains the minimum element at the root using a binary heap represented
 * as a contiguous dynamic array.
 *
 * By default, uses std::less<T>, where comp(a, b) == true indicates that
 * element 'a' has strictly higher priority (i.e. is smaller) than 'b'.
 *
 * Complexities:
 * - push():  O(log N)
 * - pop():   O(log N)
 * - top():   O(1)
 * - empty(): O(1)
 * - size():  O(1)
 * - clear(): O(1)
 */
template <typename T, typename Compare = std::less<T>>
class MinHeap {
public:
    MinHeap() = default;

    explicit MinHeap(Compare comp) : comp_(comp) {}

    template <typename InputIt>
    MinHeap(InputIt first, InputIt last, Compare comp = Compare())
        : comp_(comp), data_(first, last) {
        buildHeap();
    }

    /**
     * @brief Insert an element into the heap and bubble up to restore heap order.
     */
    void push(const T& value) {
        data_.push_back(value);
        heapifyUp(data_.size() - 1);
    }

    /**
     * @brief Move-insert an element into the heap.
     */
    void push(T&& value) {
        data_.push_back(std::move(value));
        heapifyUp(data_.size() - 1);
    }

    /**
     * @brief Access the root (minimum) element of the heap.
     * @throws std::underflow_error if the heap is empty.
     */
    const T& top() const {
        if (empty()) {
            throw std::underflow_error("Cannot access top(): MinHeap is empty.");
        }
        return data_.front();
    }

    /**
     * @brief Remove the root (minimum) element from the heap.
     * @throws std::underflow_error if the heap is empty.
     */
    void pop() {
        if (empty()) {
            throw std::underflow_error("Cannot pop(): MinHeap is empty.");
        }
        data_[0] = std::move(data_.back());
        data_.pop_back();
        if (!data_.empty()) {
            heapifyDown(0);
        }
    }

    /**
     * @brief Extract and return the root (minimum) element.
     */
    T extractMin() {
        T root = top();
        pop();
        return root;
    }

    /**
     * @brief Check whether the heap contains no elements.
     */
    bool empty() const noexcept {
        return data_.empty();
    }

    /**
     * @brief Return the number of elements in the heap.
     */
    size_t size() const noexcept {
        return data_.size();
    }

    /**
     * @brief Clear all elements from the heap.
     */
    void clear() noexcept {
        data_.clear();
    }

private:
    Compare comp_{};
    std::vector<T> data_;

    void heapifyUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (comp_(data_[index], data_[parent])) {
                std::swap(data_[index], data_[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void heapifyDown(size_t index) {
        size_t n = data_.size();
        while (true) {
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;
            size_t smallest = index;

            if (left < n && comp_(data_[left], data_[smallest])) {
                smallest = left;
            }
            if (right < n && comp_(data_[right], data_[smallest])) {
                smallest = right;
            }

            if (smallest != index) {
                std::swap(data_[index], data_[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }

    void buildHeap() {
        if (data_.size() <= 1) return;
        for (int64_t i = static_cast<int64_t>((data_.size() / 2) - 1); i >= 0; --i) {
            heapifyDown(static_cast<size_t>(i));
        }
    }
};

} // namespace codevault::dsa
