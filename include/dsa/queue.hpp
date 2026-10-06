#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace codevault::dsa {

/**
 * @brief Generic FIFO Queue implemented using a singly-linked node list.
 *
 * Guaranteed O(1) enqueue, dequeue, front, and back operations without element shifting.
 *
 * Complexities:
 * - enqueue(): O(1)
 * - dequeue(): O(1)
 * - front():   O(1)
 * - back():    O(1)
 * - empty():   O(1)
 * - size():    O(1)
 * - clear():   O(N)
 */
template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next{nullptr};

        explicit Node(const T& val) : data(val), next(nullptr) {}
        explicit Node(T&& val) : data(std::move(val)), next(nullptr) {}
    };

    Node* head_{nullptr};
    Node* tail_{nullptr};
    size_t size_{0};

public:
    Queue() = default;

    ~Queue() {
        clear();
    }

    // Copy constructor (Rule of 5)
    Queue(const Queue& other) {
        Node* curr = other.head_;
        while (curr) {
            enqueue(curr->data);
            curr = curr->next;
        }
    }

    // Copy assignment (Rule of 5)
    Queue& operator=(const Queue& other) {
        if (this != &other) {
            clear();
            Node* curr = other.head_;
            while (curr) {
                enqueue(curr->data);
                curr = curr->next;
            }
        }
        return *this;
    }

    // Move constructor (Rule of 5)
    Queue(Queue&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    // Move assignment (Rule of 5)
    Queue& operator=(Queue&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = other.head_;
            tail_ = other.tail_;
            size_ = other.size_;
            other.head_ = nullptr;
            other.tail_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /**
     * @brief Insert an element at the back of the queue (FIFO).
     */
    void enqueue(const T& value) {
        Node* newNode = new Node(value);
        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            tail_ = newNode;
        }
        ++size_;
    }

    /**
     * @brief Move-insert an element at the back of the queue.
     */
    void enqueue(T&& value) {
        Node* newNode = new Node(std::move(value));
        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            tail_ = newNode;
        }
        ++size_;
    }

    /**
     * @brief Remove the element at the front of the queue.
     * @throws std::underflow_error if queue is empty.
     */
    void dequeue() {
        if (empty()) {
            throw std::underflow_error("Cannot dequeue(): Queue is empty.");
        }
        Node* oldHead = head_;
        head_ = head_->next;
        if (!head_) {
            tail_ = nullptr;
        }
        delete oldHead;
        --size_;
    }

    /**
     * @brief Access the front element.
     * @throws std::underflow_error if queue is empty.
     */
    T& front() {
        if (empty()) {
            throw std::underflow_error("Cannot access front(): Queue is empty.");
        }
        return head_->data;
    }

    const T& front() const {
        if (empty()) {
            throw std::underflow_error("Cannot access front(): Queue is empty.");
        }
        return head_->data;
    }

    /**
     * @brief Access the back element.
     * @throws std::underflow_error if queue is empty.
     */
    T& back() {
        if (empty()) {
            throw std::underflow_error("Cannot access back(): Queue is empty.");
        }
        return tail_->data;
    }

    const T& back() const {
        if (empty()) {
            throw std::underflow_error("Cannot access back(): Queue is empty.");
        }
        return tail_->data;
    }

    /**
     * @brief Check whether queue contains no elements.
     */
    bool empty() const noexcept {
        return size_ == 0;
    }

    /**
     * @brief Return the number of elements in the queue.
     */
    size_t size() const noexcept {
        return size_;
    }

    /**
     * @brief Return the elements of the queue in FIFO order as a standard vector.
     */
    std::vector<T> toVector() const {
        std::vector<T> elements;
        elements.reserve(size_);
        Node* curr = head_;
        while (curr) {
            elements.push_back(curr->data);
            curr = curr->next;
        }
        return elements;
    }

    /**
     * @brief Check whether the queue contains an element matching value.
     */
    bool contains(const T& value) const {
        Node* curr = head_;
        while (curr) {
            if (curr->data == value) {
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    /**
     * @brief Remove the first occurrence of an element from the queue.
     * @return true if an element was found and removed, false otherwise.
     */
    bool remove(const T& value) {
        Node* curr = head_;
        Node* prev = nullptr;
        while (curr) {
            if (curr->data == value) {
                if (!prev) {
                    head_ = curr->next;
                } else {
                    prev->next = curr->next;
                }
                if (curr == tail_) {
                    tail_ = prev;
                }
                delete curr;
                --size_;
                return true;
            }
            prev = curr;
            curr = curr->next;
        }
        return false;
    }

    /**
     * @brief Remove all elements and release allocated node memory.
     */
    void clear() noexcept {
        while (head_) {
            Node* next = head_->next;
            delete head_;
            head_ = next;
        }
        tail_ = nullptr;
        size_ = 0;
    }
};

} // namespace codevault::dsa
