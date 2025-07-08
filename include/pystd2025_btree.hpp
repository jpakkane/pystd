// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2025.hpp>
#include <assert.h>

namespace pystd2025 {

template<typename T, size_t MAX_SIZE> class FixedVector {
public:
    ~FixedVector() { destroy_contents(); }

    bool try_push_back(const T &obj) noexcept {
        if(num_entries >= MAX_SIZE) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(obj);
        ++num_entries;
        return true;
    }

    bool try_push_back(T &&obj) noexcept {
        if(num_entries >= MAX_SIZE) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(::pystd2025::move(obj));
        ++num_entries;
        return true;
    }

    void push_back(const T &obj) {
        if(!try_push_back(obj)) {
            throw PyException("Tried to push back to a full FixedVector.");
        }
    }

    void push_back(T &&obj) {
        if(!try_push_back(::pystd2025::move(obj))) {
            throw PyException("Tried to push back to a full FixedVector.");
        }
    }

    void insert(size_t loc, T &&obj) { abort(); }

    size_t capacity() const noexcept { return MAX_SIZE; }

    size_t size() const noexcept { return num_entries; }

    bool is_empty() const noexcept { return size() == 0; }

    void pop_back() noexcept {
        if(num_entries == 0) {
            return;
        }
        T *obj = objptr(num_entries - 1);
        obj->~T();
        --num_entries;
    }

    void pop_front() noexcept {
        if(num_entries == 0) {
            return;
        }
        delete_at(0);
    }

    void delete_at(size_t i) {
        if(i >= size()) {
            throw PyException("OOB in delete_at.");
        }
        ++i;
        while(i < size()) {
            *objptr(i - 1) = ::pystd2025::move(*objptr(i));
        }
        pop_back();
    }

    T *data() noexcept { return reinterpret_cast<T *>(backing); }

    T &operator[](size_t i) { return *objptr(i); }

    const T &operator[](size_t i) const { return *objptr(i); }

    T *begin() const { return const_cast<T *>(objptr(0)); }
    T *end() const { return const_cast<T *>(objptr(num_entries)); }

private:
    T *objptr(size_t i) { return reinterpret_cast<T *>(rawptr(i)); }

    const T *objptr(size_t i) const { return reinterpret_cast<const T *>(rawptr(i)); }

    char *rawptr(size_t i) {
        if(i >= MAX_SIZE) {
            throw PyException("OOB in FixedVector.");
        }
        return backing + i * sizeof(T);
    }
    const char *rawptr(size_t i) const {
        if(i >= MAX_SIZE) {
            throw PyException("OOB in FixedVector.");
        }
        return backing + i * sizeof(T);
    }

    void destroy_contents() noexcept {
        for(size_t i = 0; i < num_entries; ++i) {
            objptr(i)->~T();
        }
        num_entries = 0;
    }

    char backing[MAX_SIZE * sizeof(T)] alignas(alignof(T));
    size_t num_entries = 0;
};

template<WellBehaved Payload, size_t EntryCount> class BTree {

public:
    void insert(const Payload &value) {
        Payload p(value);
        insert(::pystd2025::move(p));
    }

    void insert(Payload &&value) {
        if(is_empty()) {
            create_node(::pystd2025::move(value), NULL_REF);
            root = 0;
            num_entries = 1;
            return;
        }
        insert_recursive(value, root);
    }

    size_t size() const { return num_entries; }

    size_t is_empty() const { return size() == 0; }

private:
    static constexpr uint32_t NULL_REF = (uint32_t)-1;

    struct Node {
        uint32_t parent;
        FixedVector<Payload, EntryCount> values;
        FixedVector<uint32_t, EntryCount + 1> children;

        bool is_leaf() const {
            for(const auto &i : children) {
                if(i != NULL_REF) {
                    return false;
                }
            }
            return true;
        }
    };

    void insert_recursive(Payload &value, uint32_t current_node) {
        if(needs_to_split(current_node)) {
            current_node = split_node(current_node);
        }
        if(nodes[current_node].is_leaf()) {
            size_t insert_loc = find_insertion_point(nodes[current_node], value);
            nodes[current_node].values.insert(insert_loc, ::pystd2025::move(value));
            nodes[current_node].children.push_back(NULL_REF);
        } else {
            abort();
        }
    }

    bool needs_to_split(size_t node) const { return nodes[node].values.size() >= EntryCount - 1; }

    size_t split_node(size_t node) const { assert(false); }

    size_t find_insertion_point(const Node &node, const Payload &value) const {
        for(size_t i = 0; i < node.values.size(); ++i) {
            if(!(node.values[i] < value)) {
                return i;
            }
        }
        return node.values.size();
    }

    void create_node(Payload &&p, size_t parent_id) {
        Node n;
        n.parent = parent_id;
        n.values.try_push_back(::pystd2025::move(p));
        n.children.try_push_back(NULL_REF);
        n.children.try_push_back(NULL_REF);
        nodes.push_back(::pystd2025::move(n));
    }
    static_assert(EntryCount % 2 == 1);
    static_assert(EntryCount > 3);
    uint32_t root = NULL_REF;
    size_t num_entries = 0;
    Vector<Node> nodes;
};

} // namespace pystd2025
