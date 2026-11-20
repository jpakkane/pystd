// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd_testconfig.hpp>
#include <pystd2026/statistics.hpp>

namespace {

int breakpoint_opportunity(int number) { return number; }

} // namespace

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_mean() {
    TEST_START;
    ::pystd2026::Vector<int> data;
    data.push_back(2);
    data.push_back(4);
    data.push_back(6);

    ASSERT(!::pystd2026::mean(data.begin(), data.begin()));

    ASSERT(::pystd2026::mean(data.span()) == 4);

    return 0;
}

int test_stats() {
    printf("Testing file access.\n");
    int failing_subtests = 0;
    failing_subtests += test_mean();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_stats();
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
