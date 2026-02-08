// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_heapsort.hpp>
#include <pystd2026_mergesort.hpp>
#include <pystd_testconfig.hpp>

int breakpoint_opportunity(int number) { return number; }

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_heapsort_int() {
    TEST_START;
    int failing_subtests = 0;
    const size_t NUM_ENTRIES = 20;
    int table[NUM_ENTRIES] = {6, 2, 1, 9, 3, 16, 12, 11, 19, 13, 10, 14, 15, 17, 18, 0, 4, 5, 7, 8};

    pystd2026::heapsort(table, table + NUM_ENTRIES);

    for(size_t i = 0; i < NUM_ENTRIES; ++i) {
        ASSERT((int)i == table[i]);
    }
    return failing_subtests;
}

struct SortStruct {
    int x = -666;
    int y = -666;

    // Y does not affect sort order. A stable sort
    // should preserve the order of equal elements.
    bool operator<(const SortStruct &o) const { return x < o.x; }

    bool operator==(const SortStruct &o) const { return x == o.x; }
};

int test_mergesort() {
    TEST_START;
    pystd2026::Vector<SortStruct> items;
    const size_t NUM_VALUES = 100;
    const SortStruct last{100000, 0};
    const SortStruct first{0, 100000};
    const size_t TOTAL_SIZE = 202;

    items.emplace_back(last);
    for(size_t i = 0; i < NUM_VALUES; ++i) {
        items.emplace_back(NUM_VALUES - i, 1);
    }
    for(size_t i = 0; i < NUM_VALUES; ++i) {
        items.emplace_back(NUM_VALUES - i, 0);
    }
    items.emplace_back(first);

    ASSERT(items.size() == TOTAL_SIZE);
    pystd2026::mergesort(items.begin(), items.end());
    ASSERT(items.size() == TOTAL_SIZE);

    /*
    for(size_t i = 0; i < items.size(); ++i) {
        printf("%d %d\n", items[i].x, items[i].y);
    }
    */

    for(size_t i = 0; i < items.size() - 1; ++i) {
        ASSERT(items[i].x <= items[i + 1].x);
    }

    ASSERT(items.front() == first);
    ASSERT(items.back() == last);
    for(size_t i = 1; i < items.size() - 2; i += 2) {
        ASSERT(items[i].x == items[i + 1].x);
        ASSERT(items[i].y > items[i + 1].y);
    }
    return 0;
}

int test_heapsort() {
    int failing_subtests = 0;
    failing_subtests += test_heapsort_int();
    failing_subtests += test_mergesort();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_heapsort();
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
