// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd_config.hpp>

#ifndef PYSTD_HAS_REGEX

#include <stdio.h>

int main(int, char **) {
    printf("Regex support not enabled.\n");
    return 0;
}

#else

#include <pystd2026_regex.hpp>
#include <pystd_testconfig.hpp>

int breakpoint_opportunity(int number) { return number; }

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_u8_regex_simple() {
    TEST_START;
    pystd2026::U8String text("abcabcabc");
    pystd2026::U8String retext("(b).*?(a)");
    pystd2026::U8Regex r(retext);

    auto match = pystd2026::regex_search(r, text);
    ASSERT(match.num_groups() == 2);
    ASSERT(match.group(0) == "bca");
    ASSERT(match.group(1) == "b");
    ASSERT(match.group(2) == "a");

    return 0;
}

int test_u8_regex() {
    TEST_START;
    int failing_subtests = 0;
    failing_subtests += test_u8_regex_simple();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_u8_regex();
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

#endif
