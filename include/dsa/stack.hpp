#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace codevault::dsa {

/**
 * @brief Generic LIFO Stack custom implementation using singly linked nodes.
 *
 * Guarantees strict O(1) push, pop, and top operations without reliance on std::stack.
 *
 * Complexities:
 * - push():  O(1)
 * - pop():   O(1)
 * - top():   O(1)
 * - empty(): O(1)
 * - size():  O(1)
 * - clear(): O(N)
 */
template <typename T>
class Stack {
private:
    struct Node {
        T data;
        Node* next{nullptr};

        explicit Node(const T& val, Node* nxt = nullptr) : data(val), next(nxt) {}
        explicit Node(T&& val, Node* nxt = nullptr) : data(std::move(val)), next(nxt) {}
    };

    Node* top_{nullptr};
    size_t size_{0};

    void copyFrom(const Stack& other) {
        if (!other.top_) {
            top_ = nullptr;
            size_ = 0;
            return;
        }

        // To preserve top-to-bottom ordering during copy:
        // We first collect or recursively build the chain
        Node* otherCurr = other.top_;
        Node* myTail = nullptr;

        while (otherCurr) {
            Node* newNode = new Node(otherCurr->data, nullptr);
            if (!top_) {
                top_ = newNode;
                myTail = newNode;
            } else {
                myTail->next = newNode;
                myTail = newNode;
            }
            otherCurr = otherCurr->next;
        }
        size_ = other.size_;
    }

public:
    Stack() = default;

    ~Stack() {
        clear();
    }

    // Copy constructor (Rule of 5)
    Stack(const Stack& other) {
        copyFrom(other);
    }

    // Copy assignment (Rule of 5)
    Stack& operator=(const Stack& other) {
        if (this != &other) {
            clear();
            copyFrom(other);
        }
        return *this;
    }

    // Move constructor (Rule of 5)
    Stack(Stack&& other) noexcept
        : top_(other.top_), size_(other.size_) {
        other.top_ = nullptr;
        other.size_ = 0;
    }

    // Move assignment (Rule of 5)
    Stack& operator=(Stack&& other) noexcept {
        if (this != &other) {
            clear();
            top_ = other.top_;
            size_ = other.size_;
            other.top_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    /**
     * @brief Push an element onto the stack (LIFO).
     */
    void push(const T& value) {
        top_ = new Node(value, top_);
        ++size_;
    }

    /**
     * @brief Move-push an element onto the stack.
     */
    void push(T&& value) {
        top_ = new Node(std::move(value), top_);
        ++size_;
    }

    /**
     * @brief Remove the top element from the stack.
     * @throws std::underflow_error if the stack is empty.
     */
    void pop() {
        if (empty()) {
            throw std::underflow_error("Cannot pop(): Stack is empty.");
        }
        Node* oldTop = top_;
        top_ = top_->next;
        delete oldTop;
        --size_;
    }

    /**
     * @brief Access the top element of the stack.
     * @throws std::underflow_error if the stack is empty.
     */
    T& top() {
        if (empty()) {
            throw std::underflow_error("Cannot access top(): Stack is empty.");
        }
        return top_->data;
    }

    const T& top() const {
        if (empty()) {
            throw std::underflow_error("Cannot access top(): Stack is empty.");
        }
        return top_->data;
    }

    /**
     * @brief Check whether the stack is empty.
     */
    bool empty() const noexcept {
        return size_ == 0;
    }

    /**
     * @brief Return the number of elements in the stack.
     */
    size_t size() const noexcept {
        return size_;
    }

    /**
     * @brief Clear all elements from the stack.
     */
    void clear() noexcept {
        while (top_) {
            Node* next = top_->next;
            delete top_;
            top_ = next;
        }
        size_ = 0;
    }
};

} // namespace codevault::dsa
