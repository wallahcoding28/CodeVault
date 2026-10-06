#pragma once

#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <utility>
#include <vector>

namespace codevault::dsa {

/**
 * @brief Generic Doubly Linked List custom implementation.
 *
 * Provides bidirectional dynamic sequence traversal, O(1) head/tail insertions and deletions,
 * and O(1) node-level splicing without std::list.
 *
 * Complexities:
 * - pushFront():  O(1)
 * - pushBack():   O(1)
 * - popFront():   O(1)
 * - popBack():    O(1)
 * - insert():     O(N) by index, O(1) given node pointer
 * - remove():     O(N) by value, O(1) given node pointer
 * - removeAt():   O(N)
 * - front():      O(1)
 * - back():       O(1)
 * - empty():      O(1)
 * - size():       O(1)
 * - clear():      O(N)
 */
template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node* prev{nullptr};
        Node* next{nullptr};

        explicit Node(const T& val) : data(val), prev(nullptr), next(nullptr) {}
        explicit Node(T&& val) : data(std::move(val)), prev(nullptr), next(nullptr) {}
    };

    // Forward Iterator
    class Iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        Iterator() = default;
        explicit Iterator(Node* node) : curr_(node) {}

        reference operator*() const { return curr_->data; }
        pointer operator->() const { return &(curr_->data); }

        Iterator& operator++() {
            if (curr_) curr_ = curr_->next;
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator--() {
            if (curr_) curr_ = curr_->prev;
            return *this;
        }

        Iterator operator--(int) {
            Iterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const { return curr_ == other.curr_; }
        bool operator!=(const Iterator& other) const { return curr_ != other.curr_; }

        Node* getNode() const { return curr_; }

    private:
        Node* curr_{nullptr};
    };

    // Const Forward Iterator
    class ConstIterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = const T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        ConstIterator() = default;
        explicit ConstIterator(const Node* node) : curr_(node) {}
        ConstIterator(const Iterator& it) : curr_(it.getNode()) {}

        reference operator*() const { return curr_->data; }
        pointer operator->() const { return &(curr_->data); }

        ConstIterator& operator++() {
            if (curr_) curr_ = curr_->next;
            return *this;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        ConstIterator& operator--() {
            if (curr_) curr_ = curr_->prev;
            return *this;
        }

        ConstIterator operator--(int) {
            ConstIterator tmp = *this;
            --(*this);
            return tmp;
        }

        bool operator==(const ConstIterator& other) const { return curr_ == other.curr_; }
        bool operator!=(const ConstIterator& other) const { return curr_ != other.curr_; }

    private:
        const Node* curr_{nullptr};
    };

    DoublyLinkedList() = default;

    ~DoublyLinkedList() {
        clear();
    }

    // Copy constructor (Rule of 5)
    DoublyLinkedList(const DoublyLinkedList& other) {
        Node* curr = other.head_;
        while (curr) {
            pushBack(curr->data);
            curr = curr->next;
        }
    }

    // Copy assignment (Rule of 5)
    DoublyLinkedList& operator=(const DoublyLinkedList& other) {
        if (this != &other) {
            clear();
            Node* curr = other.head_;
            while (curr) {
                pushBack(curr->data);
                curr = curr->next;
            }
        }
        return *this;
    }

    // Move constructor (Rule of 5)
    DoublyLinkedList(DoublyLinkedList&& other) noexcept
        : head_(other.head_), tail_(other.tail_), size_(other.size_) {
        other.head_ = nullptr;
        other.tail_ = nullptr;
        other.size_ = 0;
    }

    // Move assignment (Rule of 5)
    DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept {
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
     * @brief Insert at the front of the list.
     */
    void pushFront(const T& value) {
        Node* newNode = new Node(value);
        if (!head_) {
            head_ = tail_ = newNode;
        } else {
            newNode->next = head_;
            head_->prev = newNode;
            head_ = newNode;
        }
        ++size_;
    }

    void pushFront(T&& value) {
        Node* newNode = new Node(std::move(value));
        if (!head_) {
            head_ = tail_ = newNode;
        } else {
            newNode->next = head_;
            head_->prev = newNode;
            head_ = newNode;
        }
        ++size_;
    }

    /**
     * @brief Insert at the end of the list.
     */
    void pushBack(const T& value) {
        Node* newNode = new Node(value);
        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            newNode->prev = tail_;
            tail_ = newNode;
        }
        ++size_;
    }

    void pushBack(T&& value) {
        Node* newNode = new Node(std::move(value));
        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            newNode->prev = tail_;
            tail_ = newNode;
        }
        ++size_;
    }

    /**
     * @brief Remove element from the front.
     * @throws std::underflow_error if empty.
     */
    void popFront() {
        if (empty()) {
            throw std::underflow_error("Cannot popFront(): DoublyLinkedList is empty.");
        }
        Node* oldHead = head_;
        if (head_ == tail_) {
            head_ = tail_ = nullptr;
        } else {
            head_ = head_->next;
            head_->prev = nullptr;
        }
        delete oldHead;
        --size_;
    }

    /**
     * @brief Remove element from the back.
     * @throws std::underflow_error if empty.
     */
    void popBack() {
        if (empty()) {
            throw std::underflow_error("Cannot popBack(): DoublyLinkedList is empty.");
        }
        Node* oldTail = tail_;
        if (head_ == tail_) {
            head_ = tail_ = nullptr;
        } else {
            tail_ = tail_->prev;
            tail_->next = nullptr;
        }
        delete oldTail;
        --size_;
    }

    /**
     * @brief Insert value at a specific zero-based index.
     * @throws std::out_of_range if index > size().
     */
    void insert(size_t index, const T& value) {
        if (index > size_) {
            throw std::out_of_range("Index out of range in insert().");
        }
        if (index == 0) {
            pushFront(value);
            return;
        }
        if (index == size_) {
            pushBack(value);
            return;
        }

        Node* curr = getNodeAt(index);
        Node* newNode = new Node(value);
        newNode->prev = curr->prev;
        newNode->next = curr;
        curr->prev->next = newNode;
        curr->prev = newNode;
        ++size_;
    }

    /**
     * @brief Remove the first occurrence of a value from the list.
     * @return true if an element was removed, false otherwise.
     */
    bool remove(const T& value) {
        Node* curr = head_;
        while (curr) {
            if (curr->data == value) {
                removeNode(curr);
                return true;
            }
            curr = curr->next;
        }
        return false;
    }

    /**
     * @brief Remove element at a specific zero-based index.
     * @throws std::out_of_range if index >= size().
     */
    void removeAt(size_t index) {
        if (index >= size_) {
            throw std::out_of_range("Index out of range in removeAt().");
        }
        if (index == 0) {
            popFront();
            return;
        }
        if (index == size_ - 1) {
            popBack();
            return;
        }
        Node* curr = getNodeAt(index);
        removeNode(curr);
    }

    /**
     * @brief Access front element.
     * @throws std::underflow_error if empty.
     */
    T& front() {
        if (empty()) {
            throw std::underflow_error("Cannot access front(): DoublyLinkedList is empty.");
        }
        return head_->data;
    }

    const T& front() const {
        if (empty()) {
            throw std::underflow_error("Cannot access front(): DoublyLinkedList is empty.");
        }
        return head_->data;
    }

    /**
     * @brief Access back element.
     * @throws std::underflow_error if empty.
     */
    T& back() {
        if (empty()) {
            throw std::underflow_error("Cannot access back(): DoublyLinkedList is empty.");
        }
        return tail_->data;
    }

    const T& back() const {
        if (empty()) {
            throw std::underflow_error("Cannot access back(): DoublyLinkedList is empty.");
        }
        return tail_->data;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    size_t size() const noexcept {
        return size_;
    }

    void clear() noexcept {
        Node* curr = head_;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    // Iterators
    Iterator begin() noexcept { return Iterator(head_); }
    Iterator end() noexcept { return Iterator(nullptr); }

    ConstIterator begin() const noexcept { return ConstIterator(head_); }
    ConstIterator end() const noexcept { return ConstIterator(nullptr); }

    ConstIterator cbegin() const noexcept { return ConstIterator(head_); }
    ConstIterator cend() const noexcept { return ConstIterator(nullptr); }

    /**
     * @brief Dump list elements forward into std::vector.
     */
    std::vector<T> toVector() const {
        std::vector<T> vec;
        vec.reserve(size_);
        Node* curr = head_;
        while (curr) {
            vec.push_back(curr->data);
            curr = curr->next;
        }
        return vec;
    }

    /**
     * @brief Dump list elements backward into std::vector (reverse traversal).
     */
    std::vector<T> toReverseVector() const {
        std::vector<T> vec;
        vec.reserve(size_);
        Node* curr = tail_;
        while (curr) {
            vec.push_back(curr->data);
            curr = curr->prev;
        }
        return vec;
    }

private:
    Node* head_{nullptr};
    Node* tail_{nullptr};
    size_t size_{0};

    Node* getNodeAt(size_t index) const {
        if (index < size_ / 2) {
            Node* curr = head_;
            for (size_t i = 0; i < index; ++i) {
                curr = curr->next;
            }
            return curr;
        } else {
            Node* curr = tail_;
            for (size_t i = size_ - 1; i > index; --i) {
                curr = curr->prev;
            }
            return curr;
        }
    }

    void removeNode(Node* node) {
        if (!node) return;
        if (node == head_) {
            popFront();
            return;
        }
        if (node == tail_) {
            popBack();
            return;
        }
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
        --size_;
    }
};

} // namespace codevault::dsa
