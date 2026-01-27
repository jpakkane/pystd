// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_heapsort.hpp>
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

int test_heapsort() {
    int failing_subtests = 0;
    failing_subtests += test_heapsort_int();
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
