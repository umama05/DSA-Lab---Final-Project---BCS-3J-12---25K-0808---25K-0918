//  stack.h  -  LIFO stack built on singly linked nodes

#pragma once

#include <cstddef>

template <typename T>
class Stack {
    struct Node {
        T value;
        Node* below;
        Node(const T& v, Node* b) : value(v), below(b) {}
    };

public:
    Stack() = default;
    ~Stack() { clear(); }
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    void push(const T& value) {
        top_ = new Node(value, top_);
        ++size_;
    }

    bool pop(T& out) {
        if (!top_) return false;
        Node* n = top_;
        out = n->value;
        top_ = n->below;
        delete n;
        --size_;
        return true;
    }

    const T* peek() const { return top_ ? &top_->value : nullptr; }

    bool empty() const { return size_ == 0; }
    std::size_t size() const { return size_; }

    void clear() {
        while (top_) {
            Node* n = top_;
            top_ = n->below;
            delete n;
        }
        size_ = 0;
    }

private:
    Node* top_ = nullptr;
    std::size_t size_ = 0;
};
