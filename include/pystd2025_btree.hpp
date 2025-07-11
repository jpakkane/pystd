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
        for(size_t i = num_entries - 2; i > loc; --i) {
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
            ++i;
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

    T &operator[](size_t i) {
        if(i >= num_entries) {
            throw PyException("FixedVector index out of bounds.");
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= num_entries) {
            throw PyException("Fixed Vector index out of bounds.");
        }
        return *objptr(i);
    }

    T *begin() const { return const_cast<T *>(objptr(0)); }
    T *end() const { return const_cast<T *>(objptr(num_entries)); }

private:
    T *objptr(size_t i) noexcept { return reinterpret_cast<T *>(rawptr(i)); }
    const T *objptr(size_t i) const noexcept { return reinterpret_cast<const T *>(rawptr(i)); }
    char *rawptr(size_t i) noexcept { return backing + i * sizeof(T); }
    const char *rawptr(size_t i) const noexcept { return backing + i * sizeof(T); }

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
            num_values = 1;
            return;
        }
        insert_recursive(value, root);
        validate_tree();
    }

    void remove(const Payload &value) {
        if(is_empty()) {
            return;
        }
        auto v = extract_value(value, root);
        (void)v;
        root_fixup();
    }

    const Payload *lookup(const Payload &value) const {
        if(is_empty()) {
            return nullptr;
        }
        auto current_id = root;
        while(true) {
            if(current_id == NULL_REF) {
                return nullptr;
            }
            const auto &current_node = nodes[current_id];
            auto node_loc = find_insertion_point(current_node, value);
            if(node_loc >= current_node.values.size()) {
                current_id = current_node.children.back();
            } else {
                const auto &prospective_value = current_node.values[node_loc];
                if(value < prospective_value) {
                    current_id = current_node.children[node_loc];
                } else if(prospective_value < value) {
                    current_id = current_node.children[node_loc + 1];
                } else {
                    return &prospective_value;
                }
            }
        }
        return nullptr;
    }

    size_t size() const { return num_values; }

    size_t is_empty() const { return size() == 0; }

    void debug_print(const char *msg) const {
        if(msg) {
            printf("%s\n", msg);
        }
        printf("----\nB-tree with %d as root, %d nodes and %d elements.\n\n",
               root,
               (int)nodes.size(),
               (int)size());
        int32_t node_id = -1;
        for(const auto &node : nodes) {
            ++node_id;
            printf("Node %d, parent %d size %d:\n", node_id, node.parent, (int)node.values.size());
            for(const auto &v : node.values) {
                printf(" %d", v);
            }
            printf("\n");
            for(const auto &v : node.children) {
                printf(" %d", v);
            }
            printf("\n");
        }
        printf("\n");
    }

private:
    static constexpr uint32_t NULL_REF = (uint32_t)-1;
    static constexpr uint32_t MIN_VALUE_COUNT = EntryCount / 2;
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

        void delete_at(uint32_t slot) {
            values.delete_at(slot);
            children.delete_at(slot);
        }

        void validate_node() {
            if constexpr(self_validate) {
                if(values.is_empty()) {
                    assert(children.is_empty());
                } else {
                    assert(children.size() == values.size() + 1);
                }

                for(size_t i = 0; i < values.size() - 1; ++i) {
                    assert(values[i] < values[i + 1]);
                }
                for(size_t i = 0; i < children.size() - 1; ++i) {
                    for(size_t j = i + 1; j < children.size(); ++j) {
                        if(children[i] != NULL_REF) {
                            assert(children[i] != children[j]);
                        }
                    }
                }
            }
        }
    };

    struct EntryLocation {
        uint32_t node_id;
        uint32_t offset;
    };

    void validate_tree() const {
        validate_links();
        validate_btree_props();
    }

    void validate_btree_props() const {
        for(uint32_t node_id = 0; node_id < nodes.size(); ++node_id) {
            if(node_id != root) {
                assert(nodes[node_id].values.size() >= MIN_VALUE_COUNT);
            }
        }
    }

    void validate_links() const {
        for(size_t i = 0; i < nodes.size(); ++i) {
            for(const auto &child : nodes[i].children) {
                if(child != NULL_REF) {
                    assert(nodes[child].parent == i);
                }
            }
        }
        if(!is_empty()) {
            assert(nodes[root].parent == NULL_REF);
        }
    }

    void insert_recursive(Payload &value, uint32_t current_node_id) {
        if(needs_to_split(current_node_id)) {
            current_node_id = split_node(current_node_id);
            debug_print("After split");
            validate_tree();
        }
        auto &current_node = nodes[current_node_id];
        size_t insert_loc = find_insertion_point(current_node, value);
        if(insert_loc < current_node.values.size() && current_node.values[insert_loc] == value) {
            current_node.values[insert_loc] = ::pystd2025::move(value);
            return;
        }
        if(current_node.is_leaf()) {
            current_node.values.insert(insert_loc, ::pystd2025::move(value));
            current_node.children.push_back(NULL_REF);
            current_node.validate_node();
            ++num_values;
        } else {
            if(insert_loc < current_node.values.size()) {
                if(value < current_node.values[insert_loc]) {
                    insert_recursive(value, current_node.children[insert_loc]);
                } else {
                    insert_recursive(value, current_node.children.back());
                }
            } else {
                insert_recursive(value, current_node.children[insert_loc]);
            }
        }
    }

    bool needs_to_split(size_t node) const {
        assert(node != NULL_REF);
        return nodes[node].values.is_full();
    }

    size_t split_node(size_t node_id) {
        Node &to_split_before_push = nodes[node_id];
        assert(to_split_before_push.values.is_full());
        const size_t to_right = EntryCount / 2 + 1;

        Node new_node;
        uint32_t new_node_id = nodes.size();
        new_node.parent = to_split_before_push.parent;
        for(size_t i = to_right; i < EntryCount; ++i) {
            new_node.values.push_back(pystd2025::move(to_split_before_push.values[i]));
            new_node.children.push_back(pystd2025::move(to_split_before_push.children[i]));
        }
        new_node.children.push_back(to_split_before_push.children.back());
        // Pop moved ones.
        while(to_split_before_push.values.size() > to_right) {
            to_split_before_push.children.pop_back();
            to_split_before_push.values.pop_back();
        }
        // Fix parent pointers of children.
        for(const auto child_id : new_node.children) {
            if(child_id != NULL_REF) {
                auto &child = nodes[child_id];
                assert(child.parent == node_id);
                child.parent = new_node_id;
            }
        }
        Payload value_to_move{to_split_before_push.values.back()};
        to_split_before_push.values.pop_back();
        to_split_before_push.children.pop_back();
        assert(new_node.children.size() == EntryCount / 2 + 1);
        assert(to_split_before_push.children.size() == EntryCount / 2 + 1);
        new_node.validate_node();
        to_split_before_push.validate_node();
        nodes.push_back(::pystd2025::move(new_node)); // Invalidates to_split.
        Node &to_split = nodes[node_id];
        uint32_t right_node_id = nodes.size() - 1;
        if(node_id == root) {
            Node new_root;
            new_root.parent = NULL_REF;
            new_root.values.push_back(::pystd2025::move(value_to_move));
            new_root.children.push_back(node_id);
            new_root.children.push_back(right_node_id);
            new_root.validate_node();
            nodes.push_back(::pystd2025::move(new_root));
            uint32_t new_root_id = nodes.size() - 1;
            nodes[node_id].parent = new_root_id;
            nodes[right_node_id].parent = new_root_id;
            root = new_root_id;
            return root;
        } else {
            insert_nonfull(::pystd2025::move(value_to_move), to_split.parent, right_node_id);
            return nodes[node_id].parent;
        }
    }

    void insert_nonfull(Payload &&value, uint32_t node_id, uint32_t right_id) {
        auto &node = nodes[node_id];
        assert(!node.values.is_full());
        auto insert_loc = find_insertion_point(node, value);
        if(insert_loc == node.values.size()) {
            node.values.push_back(::pystd2025::move(value));
            node.children.push_back(::pystd2025::move(right_id));
        } else {
            assert(value < node.values[insert_loc]);
            node.values.insert(insert_loc, ::pystd2025::move(value));
            node.children.insert(insert_loc + 1, ::pystd2025::move(right_id));
        }
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

    EntryLocation find_location(const Payload &value, uint32_t node_id) {
        while(true) {
            auto &node = nodes[node_id];
            auto loc = find_insertion_point(value, node_id);
            if(loc >= node.values.size()) {
                return EntryLocation{NULL_REF, NULL_REF};
            }
            if(node.values[loc] < value) {
                node_id = node.children[loc];
            } else if(!(value < node.values[loc])) {
                node_id = node.children[loc + 1];
            } else {
                return EntryLocation{node_id, loc};
            }
        }
    }

    void pop_empty_leaf(uint32_t node_id) {
        auto parent_id = nodes[node_id].parent;
        assert(parent_id != NULL_REF);
        auto &parent = nodes[parent_id];
        for(auto &c : parent.children) {
            if(c == node_id) {
                c = NULL_REF;
            }
        }
        swap_to_end_and_pop(node_id);
    }

    void swap_to_end_and_pop(uint32_t node_id) {
        assert(node_id != NULL_REF);

        // You can only pop a node if nothing points to it any
        // more. Thus we only need to repoint references to the
        // last node.
        uint32_t back_id = nodes.size() - 1;
        if(node_id != back_id) {
            const auto &to_reparent = nodes[back_id];
            if(to_reparent.parent != NULL_REF) {
                auto &grandparent = nodes[to_reparent.parent];
                for(auto &c : grandparent.children) {
                    if(c == back_id) {
                        c = node_id;
                    }
                }
            }
            for(auto c : to_reparent.children) {
                if(c != NULL_REF) {
                    nodes[c].parent = node_id;
                }
            }
            swap(nodes[node_id], nodes[back_id]);
        }
        nodes.pop_back();
    }

    Optional<Payload>
    extract_value_from_leaf(const Payload &value, uint32_t node_id, uint32_t node_loc) {
        auto &current_node = nodes[node_id];
        assert(current_node.is_leaf());
        assert(node_loc < current_node.values.size());
        assert(current_node.values.size() > MIN_VALUE_COUNT);
        if(current_node.values[node_loc] == value) {
            Optional<Payload> value = ::pystd2025::move(current_node.values[current_node]);
            current_node.delete_at(node_loc);
            return value;
        } else {
            // Value was not in tree.
            return Optional<Payload>{};
        }
    }

    Payload extract_value_from_internal(const Payload &value, uint32_t node_id, uint32_t node_loc) {
        auto &current_node = nodes[node_id];
        assert(!current_node.is_leaf());
        assert(current_node.values[node_loc] == value);
    }

    Optional<Payload> extract_value(const Payload &value, uint32_t node_id) {
        auto &current_node = nodes[node_id];
        auto node_loc = find_insertion_point(current_node, value);
        if(current_node.is_leaf()) {
            return extract_value_from_leaf(value, node_id, node_loc);
        } else {
            bool go_right;
            if(node_loc >= current_node.values.size()) {
                go_right = true;
            } else if(value < current_node.values[node_loc]) {
                go_right = false;
            } else if(current_node.values[node_loc] < value) {
                go_right = true;
            } else {
                return extract_value_from_internal(value, node_id, node_loc);
            }
            if(go_right) {
                // recurse right
            } else {
                const auto child_count = current_node.children.size();
                const uint32_t child_to_process = current_node.children[node_loc];
                const uint32_t left_sibling = node_loc > 0 ? node_loc - 1 : NULL_REF;
                const uint32_t right_sibling = node_loc < child_count - 1 ? node_loc + 1 : NULL_REF;

                if(nodes[child_to_process].size() <= MIN_VALUE_COUNT) {
                    if(left_sibling != NULL_REF &&
                       nodes[current_node.children[left_sibling]].size() > MIN_VALUE_COUNT) {
                        shift_from_left_sibling(node_id, node_loc, left_sibling, child_to_process);
                    } else if(right_sibling != NULL_REF &&
                              nodes[current_node.children[right_sibling]].size() >
                                  MIN_VALUE_COUNT) {
                        shift_from_right_sibling(
                            node_id, node_loc, child_to_process, right_sibling);
                    } else {
                        merge_siblings(
                            node_id, node_loc, child_to_process, left_sibling, right_sibling);
                    }
                }
                if(value < current_node.children[node_loc]) {
                    return extract_value(value, current_node.children[node_loc]);
                } else {
                    return extract_value(value, current_node.children[node_loc + 1]);
                }
            }
        }
    }

    void root_fixup() {
        if(is_empty()) {
            return;
        }
        if(nodes[root].values.is_empty()) {
            assert(nodes[root].children.size() == 1);
            auto new_root = nodes[root].children[0];
            swap_to_end_and_pop(root);
            root = new_root;
        }
    }

    void swap_nodes(uint32_t node_one, uint32_t node_two) {}

    static_assert(EntryCount % 2 == 1);
    static_assert(EntryCount >= 3);

    uint32_t root = NULL_REF;
    size_t num_values = 0;
    Vector<Node> nodes;
};

} // namespace pystd2025
