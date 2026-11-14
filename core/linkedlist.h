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
