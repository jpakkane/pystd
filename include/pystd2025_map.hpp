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

    void backtrack_to_next() {
        go_up();
        while(i != tree->SENTINEL_ID && direction == CameFrom::Right) {
            go_up();
        }
    }

    RBIterator &operator++() {
        if(direction == CameFrom::Top) {
            if(tree->has_right(i)) {
                i = tree->right_of(i);
                direction = CameFrom::Top;
            } else {
                backtrack_to_next();
            }
        } else if(direction == CameFrom::Left) {
            if(tree->has_right(i)) {
                i = tree->right_of(i);
                while(tree->has_left(i)) {
                    i = tree->left_of(i);
                }
                direction = CameFrom::Top;
            } else {
                backtrack_to_next();
            }
        } else {
            backtrack_to_next();
        }
        return *this;
    }

    uint32_t node_id() const { return i; }

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
        nodes.emplace_back(SENTINEL_ID, SENTINEL_ID, SENTINEL_ID, Color::Black, Key{});
        root = (uint32_t)-1;
    }

    size_t size() const { return nodes.size() - 1; }

    bool is_empty() const { return nodes.size() == 1; }

    void insert(Key key) {
        if(is_empty()) {
            nodes.emplace_back(
                SENTINEL_ID, SENTINEL_ID, SENTINEL_ID, Color::Black, pystd2025::move(key));
            root = 1;
            debug_print();
            return;
        }
        const bool need_rebalance = tree_insert(key);
        debug_print();
        if(need_rebalance) {
            insert_rebalance();
        }
        debug_print();
    }

    void remove(const Key &key) {
        auto z = lookup(key);
        if(z == SENTINEL_ID) {
            return;
        }

        auto deleted_node = RB_delete(z);
        if(deleted_node != nodes.size() - 1) {
            swap_nodes(deleted_node, nodes.size() - 1);
        }
        nodes.pop_back();
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

    uint32_t RB_delete(uint32_t z) {
        assert(z != SENTINEL_ID);
        assert(!is_empty());
        uint32_t y = SENTINEL_ID;
        if(nodes[z].left == SENTINEL_ID || nodes[z].right == SENTINEL_ID) {
            y = z;
        } else {
            y = tree_successor(z);
        }
        uint32_t x = SENTINEL_ID;
        if(left_of(y) != SENTINEL_ID) {
            x = nodes[y].left;
        } else {
            x = nodes[y].right;
        }
        nodes[x].parent = nodes[y].parent;
        if(parent_of(y) == SENTINEL_ID) {
            root = x;
        } else {
            if(y == nodes[parent_of(y)].left) {
                nodes[parent_of(y)].left = x;
            } else {
                nodes[parent_of(y)].right = x;
            }
        }
        if(y != z) {
            nodes[z].key = pystd2025::move(nodes[y].key);
        }
        if(nodes[y].is_black()) {
            RB_delete_fixup(x);
        }
        return y;
    }

    void RB_delete_fixup(uint32_t x) {
        while(x != root && nodes[x].is_black()) {
            if(left_of(parent_of(x)) == x) {
                auto w = right_of(parent_of(x));
                if(nodes[w].is_red()) {
                    nodes[w].color.set_black();
                    nodes[parent_of(x)].color.set_red();
                    left_rotate(parent_of(x));
                    w = right_of(parent_of(x));
                }
                if(left_of(w).is_black() && right_of(w).is_black()) {
                    nodes[w].set_red();
                    x = parent_of(x);
                } else {
                    if(right_of(w).is_black()) {
                        nodes[left_of(w)].set_black();
                        nodes[w].set_red();
                        right_rotate(w);
                        w = right_of(parent_of(x));
                    }
                    nodes[w].set_color(nodes[parent_of(x)].get_color());
                    nodes[parent_of(x)].set_black();
                    nodes[right_of(x)].set_black();
                    left_rotate(parent_of(x));
                    x = root;
                }
            } else {
                // FIXME, add later.
                abort();
            }
            validate_sentinel();
        }
    }

    void validate_sentinel() const {
        const auto &sentinel = nodes[SENTINEL_ID];

        assert(sentinel.parent == SENTINEL_ID);
        assert(sentinel.left == SENTINEL_ID);
        assert(sentinel.right == SENTINEL_ID);
    }

    uint32_t tree_successor(uint32_t node) {
        RBIterator<Key> it(this, node);
        ++it;
        return it.node_id();
    }

    void insert_rebalance() {
        uint32_t x = nodes.size() - 1;
        assert(!nodes[x].is_black());
        const auto &sen = nodes[SENTINEL_ID];
        while(x != root && nodes[parent_of(x)].is_red()) {
            assert(parent_of(x) != SENTINEL_ID);
            assert(parent_of(parent_of(x)) != SENTINEL_ID);
            if(parent_of(x) == left_of(parent_of(parent_of(x)))) {
                auto y = right_of(parent_of(parent_of(x)));
                if(nodes[y].is_red()) {
                    nodes[parent_of(x)].set_black();
                    nodes[y].set_black();
                    nodes[parent_of(parent_of(x))].set_red();
                    x = parent_of(parent_of(x));
                } else {
                    if(x == right_of(parent_of(x))) {
                        x = parent_of(x);
                        left_rotate(x);
                        printf("Left rotate done");
                        debug_print();
                    }
                    nodes[parent_of(x)].set_black();
                    auto gp = parent_of(parent_of(x));
                    nodes[gp].set_red();
                    right_rotate(gp);
                    printf("Right rotate done");
                    debug_print();
                }
            } else {
                auto y = left_of(parent_of(parent_of(x)));
                if(nodes[y].is_red()) {
                    nodes[parent_of(x)].set_black();
                    nodes[y].set_black();
                    nodes[parent_of(parent_of(x))].set_red();
                    x = parent_of(parent_of(x));
                } else {
                    if(x == left_of(parent_of(x))) {
                        x = parent_of(x);
                        right_rotate(x);
                    }
                    nodes[parent_of(x)].set_black();
                    auto gp = parent_of(parent_of(x));
                    nodes[gp].set_red();
                    left_rotate(gp);
                }
            }
            validate_sentinel();
            validate_nodes();
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
        const auto &xnode = nodes[x];
        auto grandparent = parent_of(x);
        auto y = right_of(x);
        const auto &ynode = nodes[y];
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
        nodes[y].parent = grandparent;

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
        const auto &ynode = nodes[y];
        auto grandparent = parent_of(y);
        auto x = left_of(y);
        const auto &xnode = nodes[x];
        // auto alpha = left_of(x);
        auto beta = right_of(x);
        // auto gamma = right_of(y);

        // nodes[x].left = alpha;
        // if(alpha != SENTINEL_ID) {
        //     nodes[alpha].parent = x;
        // }
        nodes[y].left = beta;
        if(beta != SENTINEL_ID) {
            nodes[beta].parent = y;
        }
        nodes[y].parent = x;
        nodes[x].right = y;
        //      assert(right_of(y) == gamma);
        nodes[x].parent = grandparent;

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

    void swap_nodes(uint32_t a, uint32_t b) {
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

    void debug_print() const {
        printf("\n---\nRoot: %d\n", root);
        for(uint32_t i = 0; i < nodes.size(); ++i) {
            const auto &n = nodes[i];
            printf("%u %c l: %u r: %u p: %u, value: %d\n",
                   i,
                   n.is_black() ? 'B' : 'R',
                   n.left,
                   n.right,
                   n.parent,
                   n.key);
        }
        printf("\n---\n");
    }

    void validate_nodes() const {
        for(size_t i = 1; i < nodes.size(); ++i) {
            const auto &node = nodes[i];
            if(node.left != SENTINEL_ID) {
                const auto &left = nodes[node.right];
                assert(left.parent == i);
                if(node.is_red()) {
                    assert(left.is_black());
                }
            }
            if(node.right != SENTINEL_ID) {
                const auto &right = nodes[node.right];
                assert(right.parent == i);
                if(node.is_red()) {
                    assert(right.is_black());
                }
            }
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

        Color get_color() const { return color; }

        void set_color(Color c) {
            if(c == Color::Black) {
                set_black();
            } else {
                set_red();
            }
        }
    };

    bool has_parent(uint32_t i) const { return nodes[i].parent != SENTINEL_ID; }

    bool has_left(uint32_t i) const { return nodes[i].left != SENTINEL_ID; }

    bool has_right(uint32_t i) const { return nodes[i].right != SENTINEL_ID; }

    uint32_t root;

    Vector<RBNode> nodes;
};

} // namespace pystd2025
