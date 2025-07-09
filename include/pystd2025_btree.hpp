// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2025.hpp>
#include <assert.h>

namespace pystd2025 {

template<WellBehaved T, size_t MAX_SIZE> class FixedVector {
public:
    FixedVector() noexcept = default;
    FixedVector(FixedVector &&o) noexcept { swipe_from(o); }

    ~FixedVector() { destroy_contents(); }

    FixedVector &operator=(FixedVector &&o) noexcept {
        if(this != &o) {
            destroy_contents();
            swipe_from(o);
        }
        return *this;
    }

    bool try_push_back(const T &obj) noexcept {
        if(is_full()) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(obj);
        ++num_entries;
        return true;
    }

    bool try_push_back(T &&obj) noexcept {
        if(is_full()) {
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

    void insert(size_t loc, T &&obj) {
        if(is_full()) {
            throw PyException("Insert to a full vector.");
        }
        if(loc > size()) {
            throw PyException("Insertion past the end of Fixed vector.");
        }
        if(loc == size()) {
            ++num_entries;
            new(objptr(loc)) T(::pystd2025::move(obj));
            return;
        }
        auto *last = objptr(size() - 1);
        ++num_entries;
        new(objptr(size() - 1)) T(::pystd2025::move(*last));
        for(size_t i = num_entries - 2; i > loc; ++i) {
            auto *dst = objptr(i);
            auto *src = objptr(i - 1);
            *dst = ::pystd2025::move(*src);
        }
        *objptr(loc) = ::pystd2025::move(obj);
    }

    size_t capacity() const noexcept { return MAX_SIZE; }

    size_t size() const noexcept { return num_entries; }

    bool is_empty() const noexcept { return size() == 0; }

    bool is_full() const noexcept { return size() >= MAX_SIZE; }

    void pop_back() noexcept {
        if(is_empty()) {
            return;
        }
        T *obj = objptr(num_entries - 1);
        obj->~T();
        --num_entries;
    }

    void pop_front() noexcept {
        if(is_empty()) {
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

    T &front() {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[0];
    }

    const T &front() const {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[0];
    }

    T &back() {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    const T &back() const {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[size() - 1];
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

    void swipe_from(FixedVector &o) noexcept {
        assert(this != &o);
        for(size_t i = 0; i < o.num_entries; ++i) {
            auto obj_loc = objptr(i);
            new(obj_loc) T(::pystd2025::move(*o.objptr(i)));
        }
        num_entries = o.num_entries;
        o.destroy_contents();
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
    static constexpr bool self_validate = true;

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

        void validate_node() {
            if constexpr(self_validate) {
                for(size_t i = 0; i < values.size() - 1; ++i) {
                    assert(values[i] < values[i + 1]);
                }
            }
        }
    };

    void insert_recursive(Payload &value, uint32_t current_node_id) {
        if(needs_to_split(current_node_id)) {
            current_node_id = split_node(current_node_id);
        }
        auto &current_node = nodes[current_node_id];
        if(current_node.is_leaf()) {
            size_t insert_loc = find_insertion_point(current_node, value);
            if(current_node.values[insert_loc] == value) {
                current_node.values[insert_loc] = ::pystd2025::move(value);
            } else {
                current_node.values.insert(insert_loc, ::pystd2025::move(value));
                current_node.children.push_back(NULL_REF);
                current_node.validate_node();
            }
        } else {
            abort();
        }
    }

    bool needs_to_split(size_t node) const { return nodes[node].values.is_full(); }

    size_t split_node(size_t node_id) {
        Node &to_split = nodes[node_id];
        assert(to_split.values.is_full());
        const size_t to_right = EntryCount / 2 + 1;

        Node new_node;
        new_node.parent = to_split.parent;
        for(size_t i = to_right; i < EntryCount; ++i) {
            new_node.values.push_back(pystd2025::move(to_split.values[i]));
            new_node.children.push_back(pystd2025::move(to_split.children[i]));
        }
        // Pop moved ones.
        while(to_split.children.size() > EntryCount / 2 + 1) {
            to_split.children.pop_back();
            to_split.values.pop_back();
        }
        new_node.children.push_back(pystd2025::move(to_split.children.back()));
        new_node.validate_node();
        to_split.validate_node();
        nodes.push_back(::pystd2025::move(new_node));
        uint32_t right_node_id = nodes.size() - 1;
        if(node_id == root) {
            Node new_root;
            new_root.parent = NULL_REF;
            new_root.values.push_back(::pystd2025::move(to_split.values.back()));
            to_split.values.pop_back();
            to_split.children.pop_back();
            new_root.children.push_back(node_id);
            new_root.children.push_back(right_node_id);
            new_root.validate_node();
            nodes.push_back(::pystd2025::move(new_root));
            nodes[node_id].parent = nodes.size() - 1;
            nodes[right_node_id].parent = nodes.size() - 1;
            root = nodes.size() - 1;
            return root;
        } else {
            insert_nonfull(::pystd2025::move(to_split.values.back()), node_id, right_node_id);
            to_split.values.pop_back();
            to_split.children.pop_back();
            return nodes[node_id].parent;
        }
    }

    void insert_nonfull(Payload &&value, uint32_t node_id, uint32_t right_id) {
        auto &node = nodes[node_id];
        assert(!node.values.is_full());
        auto insert_loc = find_insertion_point(node, value);
        assert(value < node.values[insert_loc]);
        node.values.insert(insert_loc, ::pystd2025::move(value));
        node.children.insert(insert_loc + 1, ::pystd2025::move(right_id));
    }

    uint32_t find_insertion_point(const Node &node, const Payload &value) const {
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
