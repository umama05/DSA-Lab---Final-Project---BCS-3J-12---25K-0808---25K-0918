//  linkedlist.h  -  singly linked list 

#pragma once

#include <cstddef>
#include <type_traits>
#include <vector>

template <typename T>
class LinkedList {
    struct Node {
        T value;
        Node* next;
        explicit Node(const T& v) : value(v), next(nullptr) {}
    };

public:
    // iterator template for regular and const_iterator
    template <bool IsConst>
    class Iter {
        friend class LinkedList;
        using NodePtr = typename std::conditional<IsConst, const Node*, Node*>::type;
        NodePtr node_;
        explicit Iter(NodePtr n) : node_(n) {}

    public:
        using reference = typename std::conditional<IsConst, const T&, T&>::type;
        using pointer = typename std::conditional<IsConst, const T*, T*>::type;

        reference operator*() const { return node_->value; }
        pointer operator->() const { return &node_->value; }
        Iter& operator++() {
            node_ = node_->next;
            return *this;
        }
        bool operator==(const Iter& o) const { return node_ == o.node_; }
        bool operator!=(const Iter& o) const { return node_ != o.node_; }
    };
    using iterator = Iter<false>;
    using const_iterator = Iter<true>;

    LinkedList() = default;
    ~LinkedList() { clear(); }

    LinkedList(const LinkedList&) = delete;
    LinkedList& operator=(const LinkedList&) = delete;
    LinkedList(LinkedList&& o) noexcept : head_(o.head_), tail_(o.tail_), size_(o.size_) {
        o.head_ = o.tail_ = nullptr;
        o.size_ = 0;
    }

    LinkedList& operator=(LinkedList&& o) noexcept {
        if (this != &o) {
            clear();
            head_ = o.head_;
            tail_ = o.tail_;
            size_ = o.size_;
            o.head_ = o.tail_ = nullptr;
            o.size_ = 0;
        }
        return *this;
    }


    void pushBack(const T& value) {
        Node* n = new Node(value);
        if (tail_) tail_->next = n;
        else head_ = n;
        tail_ = n;
        ++size_;
    }

    void pushFront(const T& value) {
        Node* n = new Node(value);
        n->next = head_;
        head_ = n;
        if (!tail_) tail_ = n;
        ++size_;
    }

    bool popFront(T* out = nullptr) {
        if (!head_) return false;
        Node* n = head_;
        if (out) *out = n->value;
        head_ = n->next;
        if (!head_) tail_ = nullptr;
        delete n;
        --size_;
        return true;
    }

    // removes the first node equal to `value`
    bool remove(const T& value) {
        Node* prev = nullptr;
        for (Node* cur = head_; cur; prev = cur, cur = cur->next) {
            if (cur->value == value) {
                unlink(prev, cur);
                return true;
            }
        }
        return false;
    }

    // removes every node for which pred(value) is true
    template <typename Pred>
    std::size_t removeIf(Pred pred) {
        std::size_t removed = 0;
        Node* prev = nullptr;
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            if (pred(cur->value)) {
                unlink(prev, cur);
                ++removed;
            }
            else {
                prev = cur;
            }
            cur = next;
        }
        return removed;
    }

    bool contains(const T& value) const {
        for (const Node* cur = head_; cur; cur = cur->next)
            if (cur->value == value) return true;
        return false;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    void clear() {
        Node* cur = head_;
        while (cur) {
            Node* next = cur->next;
            delete cur;
            cur = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }
