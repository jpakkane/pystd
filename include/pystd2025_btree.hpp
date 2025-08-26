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

    void move_append(FixedVector &o) {
        if(this == &o) {
            throw PyException("Can not move append self.");
        }
        if(size() + o.size() > MAX_SIZE) {
            throw PyException("Appending would exceed max size.");
        }
        for(auto &i : o) {
            push_back(::pystd2025::move(i));
        }
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
            create_root_node(::pystd2025::move(value));
            root = NodeReference{0, true};
            num_values = 1;
            return;
        }
        insert_recursive(value, root);
        debug_print("After insert");
        validate_tree();
    }

    void remove(const Payload &value) {
        if(is_empty()) {
            return;
        }
        const uint32_t poi = 23;
        if(value == poi) {
            debug_print("At point of interest.");
        }
        auto v = extract_value(value, root);
        if(value == poi) {
            debug_print("After point of interest.");
        }
        if(v) {
            --num_values;
        }
        root_fixup();
        validate_tree();
    }

    const Payload *lookup(const Payload &value) const {
        if(is_empty()) {
            return nullptr;
        }
        auto current_id = root;
        while(true) {
            assert(!current_id.is_null());
            const auto &current_common = get_node_common(current_id);
            auto node_loc = find_insertion_point(current_common, value);
            if(current_id.to_leaf) {
                if(node_loc >= current_common.values.size()) {
                    return nullptr;
                }
                const auto &prospective_value = current_common.values[node_loc];
                if(value < prospective_value) {
                    return nullptr;
                } else if(prospective_value < value) {
                    return nullptr;
                } else {
                    return &prospective_value;
                }
            } else {
                const auto &current_node = get_internal(current_id);
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
        }
    }

    size_t size() const { return num_values; }

    size_t is_empty() const { return size() == 0; }

    void reserve(size_t num_items) {
        auto approx_nodes = (num_items + EntryCount - 1) / EntryCount;
        internals.reserve(approx_nodes);
        leaves.reserve(approx_nodes);
    }

    bool contains(const Payload &value) const { return lookup(value); }

    void debug_print(const char *msg) const {
        if constexpr(debug_prints) {
            if(msg) {
                printf("%s\n", msg);
            }
            printf("----\nB-tree with %d:%d as root, %d internal nodes, %d leaves and %d "
                   "elements.\n\n",
                   root.id,
                   (int)root.to_leaf,
                   (int)internals.size(),
                   (int)leaves.size(),
                   (int)size());
            int32_t node_id = -1;
            printf("Internal nodes\n");
            for(const auto &node : internals) {
                ++node_id;
                printf("Inode %d:0, parent %d:%d size %d:\n",
                       node_id,
                       node.parent.id,
                       (int)node.parent.to_leaf,
                       (int)node.values.size());
                for(const auto &v : node.values) {
                    printf(" %d", v);
                }
                printf("\n");
                for(const auto &v : node.children) {
                    printf(" %d:%d", v.id, v.to_leaf);
                }
                printf("\n");
            }
            printf("\nLeaves\n");
            node_id = -1;
            for(const auto &node : leaves) {
                ++node_id;
                printf("Lnode %d:1, parent %d:%d size %d:\n",
                       node_id,
                       node.parent.id,
                       (int)node.parent.to_leaf,
                       (int)node.values.size());
                for(const auto &v : node.values) {
                    printf(" %d", v);
                }
                printf("\n");
            }
            printf("\n");
        }
    }

private:
    static constexpr uint32_t NULL_LOC = (uint32_t)-1;
    static constexpr uint32_t NULL_REF = ((uint32_t)-1) >> 1;
    static constexpr uint32_t MIN_VALUE_COUNT = EntryCount / 2;
    static constexpr bool self_validate = false;
    static constexpr bool debug_prints = false;

    struct NodeReference {
        uint32_t id : 31;
        bool to_leaf : 1;

        bool is_null() const { return id == NULL_REF; }

        static NodeReference null_ref() { return NodeReference{NULL_REF, false}; }

        bool operator==(const NodeReference &o) const { return id == o.id && to_leaf == o.to_leaf; }

        bool operator!=(const NodeReference &o) const { return !(*this == o); }
    };

    struct NodeCommon {
        NodeReference parent;
        FixedVector<Payload, EntryCount> values;

        uint32_t size() const { return values.size(); }
    };

    struct LeafNode : public NodeCommon {
        using NodeCommon::values;

        void validate_node() const {
            if constexpr(self_validate) {
                if(!values.is_empty()) {
                    for(size_t i = 0; i < values.size() - 1; ++i) {
                        assert(values[i] < values[i + 1]);
                    }
                }
            }
        }

        bool is_leaf() const { return true; }

        void pop_back() { values.pop_back(); }

        void pop_front() { values.pop_front(); }

        void delete_at(uint32_t slot) { values.delete_at(slot); }
    };

    struct InternalNode : public NodeCommon {
        using NodeCommon::values;

        FixedVector<NodeReference, EntryCount + 1> children;

        void pop_back() {
            values.pop_back();
            children.pop_back();
        }

        void pop_front() {
            values.pop_front();
            children.pop_front();
        }

        uint32_t size() const { return values.size(); }

        bool is_leaf() const { return false; }

        void delete_at(uint32_t slot) {
            values.delete_at(slot);
            children.delete_at(slot);
        }

        void validate_node() const {
            if constexpr(self_validate) {
                assert(children.size() == values.size() + 1);

                if(!values.is_empty()) {
                    for(size_t i = 0; i < values.size() - 1; ++i) {
                        assert(values[i] < values[i + 1]);
                    }
                }
                for(size_t i = 0; i < children.size() - 1; ++i) {
                    for(size_t j = i + 1; j < children.size(); ++j) {
                        if(!children[i].is_null()) {
                            assert(children[i] != children[j]);
                        }
                    }
                }
            }
        }
    };

    struct EntryLocation {
        NodeReference node_id;
        uint32_t offset;
    };

    NodeCommon &get_node_common(NodeReference ref) {
        if(ref.to_leaf) {
            return static_cast<NodeCommon &>(leaves[ref.id]);
        }
        return static_cast<NodeCommon &>(internals[ref.id]);
    }

    const NodeCommon &get_node_common(NodeReference ref) const {
        if(ref.to_leaf) {
            return static_cast<const NodeCommon &>(leaves[ref.id]);
        }
        return static_cast<const NodeCommon &>(internals[ref.id]);
    }

    InternalNode &get_internal(NodeReference ref) {
        assert(!ref.to_leaf);
        return internals[ref.id];
    }

    const InternalNode &get_internal(NodeReference ref) const {
        assert(!ref.to_leaf);
        return internals[ref.id];
    }

    LeafNode &get_leaf(NodeReference ref) {
        assert(ref.to_leaf);
        return leaves[ref.id];
    }

    const LeafNode &get_leaf(NodeReference ref) const {
        assert(ref.to_leaf);
        return leaves[ref.id];
    }

    void validate_tree() const {
        validate_nodes();
        validate_links();
        validate_btree_props();
    }

    void validate_btree_props() const {
        if(self_validate) {
            for(uint32_t inode_num = 0; inode_num < internals.size(); ++inode_num) {
                NodeReference node_id(inode_num, false);
                if(node_id != root) {
                    assert(get_internal(node_id).values.size() >= MIN_VALUE_COUNT);
                }
            }
            for(uint32_t lnode_num = 0; lnode_num < leaves.size(); ++lnode_num) {
                NodeReference node_id(lnode_num, true);
                if(node_id != root) {
                    assert(get_leaf(node_id).values.size() >= MIN_VALUE_COUNT);
                }
            }
        }
    }

    void validate_nodes() const {
        if(self_validate) {
            for(const auto &n : internals) {
                n.validate_node();
            }
            for(const auto &n : leaves) {
                n.validate_node();
            }
        }
    }

    void validate_links() const {
        if(self_validate) {
            for(size_t i = 0; i < internals.size(); ++i) {
                const auto &node = internals[i];
                assert(!node.parent.to_leaf);
                for(const auto &child : node.children) {
                    auto &c = get_node_common(child);
                    auto backlink = c.parent;
                    assert(backlink.id == i);
                }
            }
            if(!is_empty()) {
                assert(get_node_common(root).parent.is_null());
            }
        }
    }

    void insert_recursive(Payload &value, NodeReference current_node_id) {
        if(needs_to_split(current_node_id)) {
            debug_print("Before split");
            current_node_id = split_node(current_node_id);
            debug_print("After split");
            validate_tree();
        }
        auto &current_node = get_node_common(current_node_id);
        size_t insert_loc = find_insertion_point(current_node, value);
        if(insert_loc < current_node.values.size() && current_node.values[insert_loc] == value) {
            current_node.values[insert_loc] = ::pystd2025::move(value);
            return;
        }
        if(current_node_id.to_leaf) {
            current_node.values.insert(insert_loc, ::pystd2025::move(value));
            static_cast<LeafNode &>(current_node).validate_node();
            ++num_values;
        } else {
            auto &inode = static_cast<InternalNode &>(current_node);
            if(insert_loc < current_node.values.size()) {
                if(value < current_node.values[insert_loc]) {
                    insert_recursive(value, inode.children[insert_loc]);
                } else {
                    insert_recursive(value, inode.children.back());
                }
            } else {
                insert_recursive(value, inode.children[insert_loc]);
            }
        }
    }

    bool needs_to_split(NodeReference node) const {
        assert(!node.is_null());
        return get_node_common(node).values.is_full();
    }

    NodeReference split_node(NodeReference node_id) {
        if(node_id.to_leaf) {
            return split_leaf_node(node_id);
        } else {
            return split_internal_node(node_id);
        }
    }

    NodeReference split_leaf_node(NodeReference node_id) {
        const bool splitting_root = node_id == root;
        auto &to_split = get_leaf(node_id);
        assert(to_split.values.size() == EntryCount);
        NodeReference parent_id;
        if(splitting_root) {
            assert(leaves.size() == 1);
            assert(internals.size() == 0);
            InternalNode new_root;
            new_root.parent = NodeReference::null_ref();
            new_root.children.push_back(node_id);
            internals.push_back(pystd2025::move(new_root));
            parent_id = NodeReference{0, false};
            root = parent_id;
        } else {
            parent_id = to_split.parent;
        }
        NodeReference new_leaf_id{(uint32_t)leaves.size(), true};

        LeafNode new_leaf;

        new_leaf.parent = parent_id;
        to_split.parent = parent_id;
        Payload value_to_parent = pystd2025::move(to_split.values[EntryCount / 2]);
        // new_root.children.push_back(node_id);
        // new_root.children.push_back(new_leaf_id);
        for(size_t i = EntryCount / 2 + 1; i < EntryCount; ++i) {
            new_leaf.values.push_back(to_split.values[i]);
        }
        while(to_split.values.size() > (EntryCount / 2)) {
            to_split.values.pop_back();
        }
        leaves.push_back(pystd2025::move(new_leaf));

        insert_nonfull(pystd2025::move(value_to_parent), parent_id, new_leaf_id);
        return parent_id;
    }

    NodeReference split_internal_node(NodeReference node_id) {
        InternalNode &to_split_before_push = get_internal(node_id);
        assert(to_split_before_push.values.is_full());
        const size_t to_right = EntryCount / 2 + 1;

        InternalNode new_node;
        NodeReference new_node_id{(uint32_t)internals.size(), false};
        InternalNode &inode = get_internal(node_id);
        new_node.parent = to_split_before_push.parent;
        for(size_t i = to_right; i < EntryCount; ++i) {
            new_node.values.push_back(pystd2025::move(inode.values[i]));
            new_node.children.push_back(pystd2025::move(inode.children[i]));
        }
        new_node.children.push_back(inode.children.back());
        // Pop moved ones.
        while(inode.values.size() > to_right) {
            inode.children.pop_back();
            inode.values.pop_back();
        }
        // Fix parent pointers of children.
        for(const auto child_id : new_node.children) {
            if(!child_id.is_null()) {
                auto &child = get_node_common(child_id);
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
        internals.push_back(::pystd2025::move(new_node)); // Invalidates to_split_before_push.
        InternalNode &to_split = get_internal(node_id);
        NodeReference right_node_id{(uint32_t)internals.size() - 1};
        if(node_id == root) {
            InternalNode new_root;
            new_root.parent = NodeReference::null_ref();
            new_root.values.push_back(::pystd2025::move(value_to_move));
            NodeReference left_ref = node_id;
            left_ref.to_leaf = false;
            new_root.children.push_back(left_ref);
            NodeReference right_ref = right_node_id;
            right_ref.to_leaf = false;
            new_root.children.push_back(right_ref);
            new_root.validate_node();
            internals.push_back(::pystd2025::move(new_root));
            NodeReference new_root_id{(uint32_t)internals.size() - 1, false};
            get_node_common(node_id).parent = new_root_id;
            get_node_common(right_node_id).parent = new_root_id;
            root = new_root_id;
            return root;
        } else {
            insert_nonfull(::pystd2025::move(value_to_move), to_split.parent, right_node_id);
            child_was_split(to_split.parent, node_id, right_node_id);
            return get_node_common(node_id).parent;
        }
    }

    void child_was_split(NodeReference parent,
                         NodeReference node_that_was_split,
                         NodeReference new_node) {
        if(parent.is_null()) {
            return;
        }
        const bool split_is_leaf = node_that_was_split.to_leaf;
        const bool new_is_leaf = new_node.to_leaf;
        for(auto &c : get_internal(parent).children) {
            if(c.id == node_that_was_split.id) {
                c.to_leaf = split_is_leaf;
                break;
            }
        }
        for(auto &c : get_internal(parent).children) {
            if(c.id == new_node.id) {
                c.to_leaf = new_is_leaf;
                break;
            }
        }
    }

    void insert_nonfull(Payload &&value, NodeReference node_id, NodeReference right_id) {
        auto &node = get_internal(node_id);
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

    uint32_t find_insertion_point(const NodeCommon &node, const Payload &value) const {
        const auto &value_array = node.values;
        if(node.values.is_empty()) {
            return 0;
        }
        auto it = lower_bound(value_array.begin(), value_array.end(), value);

        return &(*it) - &node.values.front();
    }

    void create_root_node(Payload &&p) {
        LeafNode n;
        n.parent = NodeReference::null_ref();
        n.values.push_back(::pystd2025::move(p));
        leaves.push_back(::pystd2025::move(n));
    }

    EntryLocation find_location(const Payload &value, NodeReference node_id) {
        while(true) {
            auto &node = get_node_common(node_id);
            auto loc = find_insertion_point(value, node_id);
            if(loc >= node.values.size()) {
                return EntryLocation{NULL_REF, NULL_REF};
            }
            if(node_id.to_leaf) {
                if(node.values[loc] < value) {
                    return EntryLocation{NULL_REF, NULL_REF};
                } else if(!(value < node.values[loc])) {
                    return EntryLocation{NULL_REF, NULL_REF};
                } else {
                    return EntryLocation{node_id, loc};
                }
            } else {
                if(node.values[loc] < value) {
                    node_id = node.children[loc];
                } else if(!(value < node.values[loc])) {
                    node_id = node.children[loc + 1];
                } else {
                    return EntryLocation{node_id, loc};
                }
            }
        }
    }

    void swap_to_end_and_pop(NodeReference node_id) {
        assert(!node_id.is_null());

        if(node_id.to_leaf) {
            swap_to_end_and_pop_leaf(node_id);
        } else {
            swap_to_end_and_pop_internal(node_id);
        }
    }

    void swap_to_end_and_pop_leaf(NodeReference node_id) {
        // You can only pop a node if nothing points to it any
        // more. Thus we only need to repoint references to the
        // last node.
        NodeReference back_id;
        assert(node_id.to_leaf);
        back_id.id = (uint32_t)leaves.size() - 1;
        back_id.to_leaf = true;
        if(root == back_id) {
            root = node_id;
        }
        if(node_id != back_id) {
            const auto &to_reparent = get_leaf(back_id);
            if(!to_reparent.parent.is_null()) {
                auto &grandparent = get_internal(to_reparent.parent);
                for(auto &c : grandparent.children) {
                    if(c == back_id) {
                        c = node_id;
                    }
                }
            }
            swap(get_leaf(node_id), get_leaf(back_id));
        }
        leaves.pop_back();
    }

    void swap_to_end_and_pop_internal(NodeReference node_id) {
        // You can only pop a node if nothing points to it any
        // more. Thus we only need to repoint references to the
        // last node.
        NodeReference back_id;
        assert(!node_id.to_leaf);
        back_id.id = (uint32_t)internals.size() - 1;
        back_id.to_leaf = false;
        if(root == back_id) {
            root = node_id;
        }
        if(node_id != back_id) {
            const auto &to_reparent = get_internal(back_id);
            if(!to_reparent.parent.is_null()) {
                auto &grandparent = get_internal(to_reparent.parent);
                for(auto &c : grandparent.children) {
                    if(c == back_id) {
                        c = node_id;
                    }
                }
            }
            for(auto c : to_reparent.children) {
                if(!c.is_null()) {
                    get_node_common(c).parent = node_id;
                }
            }
            swap(get_internal(node_id), get_internal(back_id));
        }
        internals.pop_back();
    }

    Optional<Payload>
    extract_value_from_leaf(const Payload &value, NodeReference node_id, uint32_t node_loc) {
        auto &current_node = get_leaf(node_id);
        assert(current_node.is_leaf());
        assert(node_loc < current_node.values.size() + 1);
        assert(node_id == root || current_node.values.size() > MIN_VALUE_COUNT);
        if(node_loc >= current_node.values.size()) {
            return Optional<Payload>{};
        }
        const auto &prospective = current_node.values[node_loc];
        if(!(prospective < value) && !(value < prospective)) {
            Optional<Payload> value = ::pystd2025::move(current_node.values[node_loc]);
            current_node.delete_at(node_loc);
            return value;
        } else {
            // Value was not in tree.
            return Optional<Payload>{};
        }
    }

    Payload
    extract_value_from_internal(const Payload &value, NodeReference node_id, uint32_t node_loc) {
        auto &p = get_internal(node_id);
        assert(!p.is_leaf());
        assert(p.values[node_loc] == value);
        auto lc_id = p.children[node_loc];
        auto &lc = get_node_common(lc_id);
        auto rc_id = p.children[node_loc + 1];
        auto &rc = get_node_common(rc_id);

        if(lc.size() > MIN_VALUE_COUNT) {
            auto predecessor_id = find_predecessor(lc_id);
            auto &predecessor = get_node_common(predecessor_id);
            Payload return_value = ::pystd2025::move(p.values[node_loc]);
            p.values[node_loc] = ::pystd2025::move(predecessor.values.back());
            if(predecessor_id.to_leaf) {
                static_cast<LeafNode &>(predecessor).pop_back();
            } else {
                static_cast<InternalNode &>(predecessor).pop_back();
            }
            return return_value;
        } else if(rc.size() > MIN_VALUE_COUNT) {
            auto successor_id = find_successor(rc_id);
            auto &successor = get_node_common(successor_id);
            Payload return_value = ::pystd2025::move(p.values[node_loc]);
            p.values[node_loc] = ::pystd2025::move(successor.values.front());
            if(successor_id.to_leaf) {
                static_cast<LeafNode &>(successor).pop_front();
            } else {
                static_cast<InternalNode &>(successor).pop_front();
            }
            return return_value;
        } else {
            debug_print("Before merge.");
            merge_siblings_of_entry(node_id, node_loc);
            auto sub_value = extract_value(value, lc_id);
            assert(sub_value);
            return ::pystd2025::move(sub_value.value());
        }
        abort();
    }

    NodeReference find_predecessor(NodeReference node_id) const {
        while(!node_id.to_leaf) {
            node_id = get_internal(node_id).children.back();
        }
        return node_id;
    }

    NodeReference find_successor(NodeReference node_id) const {
        while(!node_id.to_leaf) {
            node_id = get_internal(node_id).children.front();
        }
        return node_id;
    }

    struct NodeSearchResult {
        uint32_t loc;
        bool has_query_value = false;
        Optional<uint32_t> child_loc;
    };

    NodeSearchResult search_node(const Payload &value, NodeReference node_id) {
        auto &current_node = get_node_common(node_id);
        NodeSearchResult result;
        result.loc = find_insertion_point(current_node, value);
        if(result.loc == current_node.values.size()) {
            result.has_query_value = false;
            if(!node_id.to_leaf) {
                result.child_loc = static_cast<InternalNode &>(current_node).children.size() - 1;
            }
            return result;
        }
        const auto &current_value = current_node.values[result.loc];
        result.has_query_value = (!(value < current_value)) && (!(current_value < value));
        if(!result.has_query_value) {
            result.child_loc = value < current_value ? result.loc : result.loc + 1;
        }
        return result;
    }

    Optional<Payload> extract_value(const Payload &value, NodeReference node_id) {
        NodeSearchResult search_result = search_node(value, node_id);
        if(node_id.to_leaf) {
            // Case 1 in Cormen-Leiserson-Rivest
            return extract_value_from_leaf(value, node_id, search_result.loc);
        }
        if(search_result.has_query_value) {
            // Case 2 in Cormen-Leiserson-Rivest.
            return extract_value_from_internal(value, node_id, search_result.loc);
        }
        auto &current_node = get_internal(node_id);
        // Case 3 in Cormen-Leiserson-Rivest
        const auto child_count = current_node.children.size();

        const uint32_t child_loc_to_process =
            search_result.child_loc ? *search_result.child_loc : current_node.children.size() - 1;
        const NodeReference child_to_process = current_node.children[child_loc_to_process];

        if(get_node_common(child_to_process).size() <= MIN_VALUE_COUNT) {
            const uint32_t left_sibling_loc =
                child_loc_to_process > 0 ? child_loc_to_process - 1 : NULL_LOC;
            const uint32_t right_sibling_loc =
                child_loc_to_process < child_count - 1 ? child_loc_to_process + 1 : NULL_LOC;

            const NodeReference left_sibling_id = left_sibling_loc != NULL_LOC
                                                      ? current_node.children[left_sibling_loc]
                                                      : NodeReference::null_ref();
            const NodeReference right_sibling_id = right_sibling_loc != NULL_LOC
                                                       ? current_node.children[right_sibling_loc]
                                                       : NodeReference::null_ref();

            if(!left_sibling_id.is_null() &&
               get_node_common(left_sibling_id).size() > MIN_VALUE_COUNT) {
                shift_node_to_right(node_id, search_result.loc - 1);
            } else if(!right_sibling_id.is_null() &&
                      get_node_common(right_sibling_id).size() > MIN_VALUE_COUNT) {
                shift_node_to_left(node_id, search_result.loc);
            } else {
                NodeReference merged_node_id;
                if(search_result.loc == current_node.values.size()) {
                    merged_node_id =
                        merge_siblings_of_entry(node_id, current_node.values.size() - 1);
                } else {
                    merged_node_id = merge_siblings_of_entry(node_id, search_result.loc);
                }
                return extract_value(value, merged_node_id);
            }
        }
        return extract_value(value, child_to_process);
    }

    void shift_node_to_right(NodeReference node_id, uint32_t node_loc) {
        auto &p = get_internal(node_id);
        const auto left_sibling_id = p.children[node_loc];
        auto &lc = get_node_common(left_sibling_id);
        const auto right_sibling_id = p.children[node_loc + 1];
        auto &rc = get_node_common(right_sibling_id);
        // A property of a B-tree is that every leaf node is at
        // the same height.
        assert(left_sibling_id.to_leaf == right_sibling_id.to_leaf);
        if(left_sibling_id.to_leaf) {
            auto &l = static_cast<LeafNode &>(lc);
            auto &r = static_cast<LeafNode &>(rc);
            r.values.insert(0, pystd2025::move(p.values[node_loc]));
            p.values[node_loc] = pystd2025::move(l.values.back());
            l.values.pop_back();
        } else {
            auto &l = static_cast<InternalNode &>(lc);
            auto &r = static_cast<InternalNode &>(rc);
            NodeReference to_reparent_id = l.children.back();
            r.values.insert(0, pystd2025::move(p.values[node_loc]));
            p.values[node_loc] = pystd2025::move(l.values.back());
            r.children.insert(0, pystd2025::move(l.children.back()));
            if(!to_reparent_id.is_null()) {
                assert(get_node_common(to_reparent_id).parent == left_sibling_id);
                get_node_common(to_reparent_id).parent = right_sibling_id;
            }
            l.values.pop_back();
            l.children.pop_back();
        }
    }

    void shift_node_to_left(NodeReference node_id, uint32_t node_loc) {
        auto &p = get_internal(node_id);
        const auto left_sibling_id = p.children[node_loc];
        auto &lc = get_node_common(left_sibling_id);
        const auto right_sibling_id = p.children[node_loc + 1];
        auto &rc = get_node_common(right_sibling_id);
        assert(left_sibling_id.to_leaf == right_sibling_id.to_leaf);

        if(left_sibling_id.to_leaf) {
            auto &l = static_cast<LeafNode &>(lc);
            auto &r = static_cast<LeafNode &>(rc);
            l.values.push_back(pystd2025::move(p.values[node_loc]));
            p.values[node_loc] = pystd2025::move(r.values.front());
            r.values.pop_front();
        } else {
            auto &l = static_cast<InternalNode &>(lc);
            auto &r = static_cast<InternalNode &>(rc);
            const auto to_reparent_id = r.children.front();

            l.values.push_back(pystd2025::move(p.values[node_loc]));
            p.values[node_loc] = pystd2025::move(r.values.front());
            l.children.push_back(pystd2025::move(r.children.front()));
            if(!to_reparent_id.is_null()) {
                assert(get_node_common(to_reparent_id).parent == right_sibling_id);
                get_node_common(to_reparent_id).parent = left_sibling_id;
            }
            r.values.pop_front();
            r.children.pop_front();
        }
    }

    void reset_parent_for_children(NodeReference node_id) {
        if(node_id.to_leaf) {
            return;
        }
        for(auto &c_id : get_internal(node_id).children) {
            if(!c_id.is_null()) {
                get_node_common(c_id).parent = node_id;
            }
        }
    }

    NodeReference merge_siblings_of_entry(NodeReference node_id, uint32_t node_loc) {
        auto &p = get_internal(node_id);
        assert(node_loc < p.values.size());
        const NodeReference left_sibling_id = p.children[node_loc];
        const NodeReference right_sibling_id = p.children[node_loc + 1];

        const bool merging_leaves = left_sibling_id.to_leaf;
        auto &l = get_node_common(left_sibling_id);
        auto &r = get_node_common(right_sibling_id);
        assert(l.values.size() + r.values.size() + 1 <= EntryCount);
        l.values.push_back(p.values[node_loc]);
        p.values.delete_at(node_loc);
        p.children.delete_at(node_loc + 1);
        l.values.move_append(r.values);
        if(!merging_leaves) {
            static_cast<InternalNode &>(l).children.move_append(
                static_cast<InternalNode &>(r).children);
        }
        const bool left_sibling_is_last =
            left_sibling_id.id == (merging_leaves ? leaves.size() - 1 : internals.size() - 1);
        reset_parent_for_children(left_sibling_id);
        swap_to_end_and_pop(right_sibling_id);
        if(left_sibling_is_last) {
            return right_sibling_id;
        }
        return left_sibling_id;
    }

    void root_fixup() {
        if(is_empty()) {
            return;
        }
        auto &root_ref = get_node_common(root);
        if(!root_ref.values.is_empty()) {
            return;
        }
        if(!root.to_leaf) {
            auto &root_inner = static_cast<InternalNode &>(root_ref);
            assert(root_inner.children.size() == 1);
            auto new_root = root_inner.children[0];
            // FIXME new_root might have changed because of the pop?
            swap_to_end_and_pop(root);
            root = new_root;
            get_node_common(root).parent = NodeReference::null_ref();
        } else {
            abort();
        }
    }

    static_assert(EntryCount % 2 == 1);
    static_assert(EntryCount >= 3);

    NodeReference root = NodeReference::null_ref();
    size_t num_values = 0;
    Vector<InternalNode> internals;
    Vector<LeafNode> leaves;
};

template<WellBehaved Key, size_t EntrySize> class BTreeSet {
public:
    BTreeSet() noexcept = default;

    void insert(const Key &value) { tree.insert(value); }

    bool contains(const Key &value) { return tree.contains(value); }

    void remove(const Key &value) { tree.remove(value); }

    size_t size() const noexcept { return tree.size(); }

    bool is_empty() const noexcept { return tree.is_empty(); }

private:
    BTree<Key, EntrySize> tree;
};

template<WellBehaved Key, WellBehaved Value, size_t EntrySize> class BTreeMap {
private:
    struct MapEntry {
        Key key;
        Value value;
        bool operator==(const MapEntry &o) const noexcept { return key == o.key; }
        bool operator<(const MapEntry &o) const noexcept { return key < o.key; }

        bool operator==(const Key &k) const noexcept { return k == key; }
        bool operator<(const Key &k) const noexcept { return k <= key; }
    };

public:
    BTreeMap() noexcept = default;

    // These make a copy of the key object. Fix at some point.
    void insert(const Key &key, Value v) {
        MapEntry entry{key, pystd2025::move(v)};
        tree.insert(pystd2025::move(entry));
    }

    Value *lookup(const Key &key) {
        MapEntry entry{key, Value{}};
        auto *loc = tree.lookup(entry);
        if(loc) {
            return &(loc->value);
        }
        return nullptr;
    }

    void remove(const Key &key) {
        MapEntry entry{key, Value{}};
        tree.remove(pystd2025::move(entry));
    }

private:
    BTree<MapEntry, EntrySize> tree;
};

} // namespace pystd2025
