// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_btree.hpp>
#include <pystd_testconfig.hpp>

int breakpoint_opportunity(int number) { return number; }

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_btree1() {
    TEST_START;
    const int shuffled[] = {7,  17, 19, 24, 2, 20, 14, 1,  6, 23, 8, 12, 25,
                            21, 15, 22, 5,  0, 18, 4,  16, 3, 11, 9, 13, 10};
    const int arr_size = 26;
    pystd2026::BTree<int, 5> btree;
    ASSERT(btree.is_empty());
    for(int i = 0; i < arr_size; ++i) {
        btree.insert(shuffled[i]);
        for(int j = 0; j < arr_size; ++j) {
            auto *valptr = btree.lookup(shuffled[j]);
            if(j <= i) {
                ASSERT(valptr);
                ASSERT(*valptr == shuffled[j]);
            } else {
                ASSERT(!valptr);
            }
        }
    }
    ASSERT(btree.size() == 26);
    btree.insert(7);
    ASSERT(btree.size() == 26);

    ASSERT(!btree.lookup(100));

    size_t expected_size = 26;

    ASSERT(btree.size() == expected_size);
    for(int i = 0; i < arr_size; ++i) {
        btree.remove(shuffled[i]);
        --expected_size;
        ASSERT(btree.size() == expected_size);
        for(int j = 0; j < arr_size; ++j) {
            auto *valptr = btree.lookup(shuffled[j]);
            if(j <= i) {
                ASSERT(!valptr);
            } else {
                ASSERT(valptr);
                ASSERT(*valptr == shuffled[j]);
            }
        }
    }
    ASSERT(btree.is_empty());

    return 0;
}

int test_btree_iteration() {
    TEST_START;
    pystd2026::BTree<int, 5> btree;
    for(int i = 99; i >= 0; --i) {
        btree.insert(i);
    }

    int expected = 0;
    for(const auto &val : btree) {
        ASSERT(val == expected);
        ++expected;
    }
    ASSERT(expected == 100);
    return 0;
}

int test_btree() {
    printf("Testing Btree.\n");
    int failing_subtests = 0;
    failing_subtests += test_btree1();
    failing_subtests += test_btree_iteration();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_btree();
    } catch(const pystd2026::PyException &e) {
        printf("Testing failed: %s\n", e.what().c_str());
        return 42;
    }

    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
