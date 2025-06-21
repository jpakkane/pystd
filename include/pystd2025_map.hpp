// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2025.hpp>
#include <assert.h>

namespace pystd2025 {

// In its own file as nobody other than me should use this yet.

// Nomenclature as in Cormen-Leiserson-Rivest.

template<WellBehaved Key> class RBTree;

template<WellBehaved Key> class RBIterator {
public:
    RBIterator(RBTree<Key> *tree_, uint32_t node_id)
        : tree{tree_}, i{node_id}, direction{CameFrom::Top} {}

    bool operator==(const RBIterator &o) const { return i == o.i; }

    bool operator!=(const RBIterator &o) const { return !(*this == o); }

    Key &operator*() { return tree->nodes[i].key; }

    RBIterator &operator++() {
        if(direction == CameFrom::Top) {
            if(tree->has_right(i)) {
                i = tree->right_of(i);
                direction = CameFrom::Top;
            } else {
                go_up();
            }
        } else if(direction == CameFrom::Left) {
            if(tree->has_right(i)) {
                i = tree->right_of(i);
                while(tree->has_left(i)) {
                    i = tree->left_of(i);
                }
                direction = CameFrom::Top;
            } else {
                go_up();
            }
        } else {
            go_up();
            while(i != tree->SENTINEL_ID && direction == CameFrom::Right) {
                go_up();
            }
        }
        return *this;
    }

private:
    void go_up() {
        auto p = tree->parent_of(i);
        auto dir = tree->left_of(p) == i ? CameFrom::Left : CameFrom::Right;
        i = p;
        direction = dir;
    }

    enum class CameFrom : uint8_t { Top, Left, Right };

    RBTree<Key> *tree;
    uint32_t i;
    CameFrom direction;
};

template<WellBehaved Key> class RBTree {
public:
    friend class RBIterator<Key>;
    RBTree() {
        nodes.emplace_back((uint32_t)-1, (uint32_t)-1, (uint32_t)-1, Color::Black, Key{});
        root = (uint32_t)-1;
    }

    size_t size() const { return nodes.size() - 1; }

    bool is_empty() const { return nodes.size() == 1; }

    void insert(Key key) {
        if(is_empty()) {
            nodes.emplace_back(
                SENTINEL_ID, SENTINEL_ID, SENTINEL_ID, Color::Black, pystd2025::move(key));
            root = 1;
            return;
        }
        const bool need_rebalance = tree_insert(key);
        if(need_rebalance) {
            insert_rebalance();
        }
    }

    RBIterator<Key> begin() {
        auto i = root;
        if(is_empty()) {
            return RBIterator{this, SENTINEL_ID};
        }
        while(left_of(i) != SENTINEL_ID) {
            i = left_of(i);
        }
        return RBIterator{this, i};
    }

    RBIterator<Key> end() { return RBIterator{this, SENTINEL_ID}; }

private:
    uint32_t left_of(uint32_t i) const { return nodes[i].left; }

    uint32_t right_of(uint32_t i) const { return nodes[i].right; }

    uint32_t parent_of(uint32_t i) const { return nodes[i].parent; }

    void insert_rebalance() {
        uint32_t x = nodes.size() - 1;
        assert(!nodes[x].is_black());
        while(x != root && !nodes[parent_of(x)].is_black()) {
            assert(parent_of(x) != SENTINEL_ID);
            assert(parent_of(parent_of(x)) != SENTINEL_ID);
            if(parent_of(x) == left_of(parent_of(parent_of(x)))) {
                auto y = right_of(parent_of(parent_of(x)));
                if(nodes[y].is_black()) {
                    nodes[parent_of(x)].set_black();
                    nodes[y].set_black();
                    nodes[parent_of(parent_of(x))].set_red();
                    x = parent_of(parent_of(x));
                } else if(x == right_of(parent_of(x))) {
                    x = parent_of(x);
                    left_rotate(x);
                    nodes[parent_of(x)].set_black();
                    nodes[parent_of(parent_of(x))].set_red();
                    right_rotate(parent_of(parent_of(x)));
                }
            } else {
                auto y = left_of(parent_of(parent_of(x)));
                if(nodes[y].is_black()) {
                    nodes[parent_of(x)].set_black();
                    nodes[y].set_black();
                    nodes[parent_of(parent_of(x))].set_red();
                    x = parent_of(parent_of(x));
                } else if(x == left_of(parent_of(x))) {
                    x = parent_of(x);
                    right_rotate(x);
                    nodes[parent_of(x)].set_black();
                    nodes[parent_of(parent_of(x))].set_red();
                    left_rotate(parent_of(parent_of(x)));
                }
            }
        }
        nodes[root].set_black();
    }

    bool tree_insert(Key &key) {
        auto current_node = root;
        while(true) {
            if(key < nodes[current_node].key) {
                if(!has_left(current_node)) {
                    nodes[current_node].left = nodes.size();
                    nodes.emplace_back(
                        SENTINEL_ID, SENTINEL_ID, current_node, Color::Red, pystd2025::move(key));
                    return true;
                } else {
                    current_node = nodes[current_node].left;
                }
            } else if(key == nodes[current_node].key) {
                return false;
            } else {
                if(!has_right(current_node)) {
                    nodes[current_node].right = nodes.size();
                    nodes.emplace_back(
                        SENTINEL_ID, SENTINEL_ID, current_node, Color::Red, pystd2025::move(key));
                    return true;
                } else {
                    current_node = nodes[current_node].right;
                }
            }
        }
    }

    uint32_t lookup(const Key &key) const {
        if(is_empty()) {
            return SENTINEL_ID;
        }
        uint32_t current_node = root;
        while(true) {
            if(key < nodes[current_node].key) {
                if(!has_left(current_node)) {
                    return SENTINEL_ID;
                } else {
                    current_node = nodes[current_node].left;
                }
            } else if(key == nodes[current_node].key) {
                return current_node;
            } else {
                if(!has_right(current_node)) {
                    return SENTINEL_ID;
                } else {
                    current_node = nodes[current_node].right;
                }
            }
        }
    }

    void left_rotate(uint32_t x) {
        assert(right_of(x) != SENTINEL_ID);
        auto grandparent = parent_of(x);
        auto y = right_of(x);
        // auto alpha = left_of(x);
        auto beta = left_of(y);
        // auto gamma = right_of(y);

        // nodes[x].left = alpha;
        // if(alpha != SENTINEL_ID) {
        //     nodes[alpha].parent = x;
        // }
        nodes[x].right = beta;
        if(beta != SENTINEL_ID) {
            nodes[beta].parent = x;
        }
        nodes[x].parent = y;
        nodes[y].left = x;
        //      assert(right_of(y) == gamma);

        if(grandparent == SENTINEL_ID) {
            root = y;
        } else {
            if(left_of(grandparent) == x) {
                nodes[grandparent].left = y;
            } else {
                nodes[grandparent].right = y;
            }
        }
    }

    void right_rotate(uint32_t y) {
        assert(left_of(y) != SENTINEL_ID);
        auto grandparent = parent_of(y);
        auto x = left_of(y);
        // auto alpha = left_of(x);
        auto beta = right_of(x);
        // auto gamma = right_of(y);

        // nodes[x].left = alpha;
        // if(alpha != SENTINEL_ID) {
        //     nodes[alpha].parent = x;
        // }
        nodes[y].left = y;
        if(beta != SENTINEL_ID) {
            nodes[beta].parent = y;
        }
        nodes[y].parent = x;
        nodes[x].right = y;
        //      assert(right_of(y) == gamma);

        if(grandparent == SENTINEL_ID) {
            root = x;
        } else {
            if(left_of(grandparent) == y) {
                nodes[grandparent].left = x;
            } else {
                nodes[grandparent].right = x;
            }
        }
    }

    void swap(uint32_t a, uint32_t b) {
        assert(a != SENTINEL_ID);
        assert(b != SENTINEL_ID);
        assert(a < nodes.size());
        assert(b < nodes.size());

        auto a_p = nodes[a].p;
        auto a_l = nodes[a].l;
        auto a_r = nodes[a].r;

        auto b_p = nodes[b].p;
        auto b_l = nodes[b].l;
        auto b_r = nodes[b].r;

        // Parents
        if(a_p != SENTINEL_ID) {
            swap_child_for(a_p, a, b);
        }
        if(b_p != SENTINEL_ID) {
            swap_child_for(b_p, b, a);
        }

        // Node pointers.
        nodes[a].parent = b_p;
        nodes[a].left = b_l;
        nodes[a].right = b_r;

        nodes[b].parent = a_p;
        nodes[b].left = a_l;
        nodes[b].right = a_r;

        // And finally the value.
        pystd2025::swap(nodes[a].key, nodes[b].key);
        // pystd2025::swap(nodes[a].value, nodes[b].value);
    }

    void swap_child_for(uint32_t parent, uint32_t old_child, uint32_t new_child) {
        if(nodes[parent].left == old_child) {
            nodes[parent].left = new_child;
        } else {
            assert(nodes[parent].right == old_child);
            nodes[parent].right = new_child;
        }
    }

    static constexpr uint32_t SENTINEL_ID = 0;

    enum class Color : uint8_t {
        Red,
        Black,
    };

    struct RBNode {
        uint32_t left;
        uint32_t right;
        uint32_t parent;
        Color color;
        Key key;
        // Value value;

        // In the future the color will be stashed inside
        // some other field. Thus use these helpers
        // consistently
        bool is_black() const { return color == Color::Black; }

        bool is_red() const { return color == Color::Red; }

        void set_black() { color = Color::Black; }

        void set_red() { color = Color::Red; }
    };

    bool has_parent(uint32_t i) const { return nodes[i].parent != SENTINEL_ID; }

    bool has_left(uint32_t i) const { return nodes[i].left != SENTINEL_ID; }

    bool has_right(uint32_t i) const { return nodes[i].right != SENTINEL_ID; }

    uint32_t root;

    Vector<RBNode> nodes;
};

} // namespace pystd2025
