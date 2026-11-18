//  hashtable.h  -  hash table, separate chaining, keyed by integer id

#pragma once

#include <cstddef>
#include <cstdint>

template <typename V>
class HashTable {
    struct Node {
        int key;
        V value;
        Node* next;
        Node(int k, const V& v) : key(k), value(v), next(nullptr) {}
    };

public:
    explicit HashTable(std::size_t initialCapacity = 16)
        : capacity_(initialCapacity < 8 ? 8 : initialCapacity) {
        buckets_ = new Node*[capacity_]();
    }
    ~HashTable() {
        freeAll();
        delete[] buckets_;
    }
    HashTable(const HashTable&) = delete;
    HashTable& operator=(const HashTable&) = delete;

    // inserts a new key, returns a pointer to the stored value, 
    V* insert(int key, const V& value) {
        if (find(key)) return nullptr;
        if (static_cast<double>(size_ + 1) > kMaxLoad * static_cast<double>(capacity_)) grow();
        Node* n = new Node(key, value);
        std::size_t b = indexFor(key, capacity_);
        n->next = buckets_[b];  
        buckets_[b] = n;
        ++size_;
        return &n->value;
    }

    V* find(int key) {
        for (Node* n = buckets_[indexFor(key, capacity_)]; n; n = n->next)
            if (n->key == key) return &n->value;
        return nullptr;
    }
    const V* find(int key) const {
        for (const Node* n = buckets_[indexFor(key, capacity_)]; n; n = n->next)
            if (n->key == key) return &n->value;
        return nullptr;
    }

    bool contains(int key) const { return find(key) != nullptr; }

    bool remove(int key) {
        std::size_t b = indexFor(key, capacity_);
        Node* prev = nullptr;
        for (Node* n = buckets_[b]; n; prev = n, n = n->next) {
            if (n->key == key) {
                if (prev) prev->next = n->next;
                else buckets_[b] = n->next;
                delete n;
                --size_;
                return true;
            }
        }
        return false;
    }

    void clear() {
        freeAll();
        size_ = 0;
    }

    std::size_t size() const { return size_; }
    std::size_t bucketCount() const { return capacity_; }
    double loadFactor() const { return static_cast<double>(size_) / static_cast<double>(capacity_); }

    template <typename F>
    void forEach(F f) {
        for (std::size_t i = 0; i < capacity_; ++i)
            for (Node* n = buckets_[i]; n; n = n->next) f(n->key, n->value);
    }
    template <typename F>
    void forEach(F f) const {
        for (std::size_t i = 0; i < capacity_; ++i)
            for (const Node* n = buckets_[i]; n; n = n->next) f(n->key, n->value);
    }

private:
    static constexpr double kMaxLoad = 0.75;

    static std::size_t indexFor(int key, std::size_t capacity) {
        std::uint32_t h = static_cast<std::uint32_t>(key) * 2654435761u;
        return static_cast<std::size_t>(h) % capacity;
    }

    void grow() {
        std::size_t newCap = capacity_ * 2;
        Node** nb = new Node*[newCap]();
        for (std::size_t i = 0; i < capacity_; ++i) {
            Node* n = buckets_[i];
            while (n) {
                Node* next = n->next;
                std::size_t b = indexFor(n->key, newCap);
                n->next = nb[b];
                nb[b] = n;
                n = next;
            }
        }
        delete[] buckets_;
        buckets_ = nb;
        capacity_ = newCap;
    }

    void freeAll() {
        for (std::size_t i = 0; i < capacity_; ++i) {
            Node* n = buckets_[i];
            while (n) {
                Node* next = n->next;
                delete n;
                n = next;
            }
            buckets_[i] = nullptr;
        }
    }

    Node** buckets_ = nullptr;
    std::size_t capacity_;
    std::size_t size_ = 0;
};
