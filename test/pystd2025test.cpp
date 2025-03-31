// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <string.h>

namespace {

const char daikatana[] = "大刀";

}

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s\n", message);                                                                   \
        return 1;                                                                                  \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.\n");

int test_string_simple() {
    pystd2025::U8String str("abc");
    ASSERT_WITH(strcmp("abc", str.c_str()) == 0, "String construction failed.");
    return 0;
}

int test_u8_iterator() {
    pystd2025::U8String ascii("abc");
    auto start = ascii.cbegin();
    auto end = ascii.cend();
    ASSERT_WITH(start != end, "Iterator construction fail.");
    ASSERT(*start == 'a');
    ++start;
    ASSERT(*start == 'b');
    start++;
    ASSERT(*start == 'c');
    ASSERT(start != end);
    ++start;
    ASSERT(start == end);
    return 0;
}

int test_u8_iterator_cjk() {
    pystd2025::U8String cjk(daikatana);
    auto it = cjk.cbegin();
    auto c1 = *it;
    ASSERT(c1 == 22823);
    ++it;
    auto c2 = *it;
    ASSERT(c2 == 20992);
    ++it;
    ASSERT(it == cjk.cend());
    return 0;
}

int test_strings() {
    printf("Testing U8strings.\n");
    int failing_subtests = 0;
    failing_subtests += test_string_simple();
    failing_subtests += test_u8_iterator();
    failing_subtests += test_u8_iterator_cjk();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_strings();
    } catch(const pystd2025::PyException &e) {
        printf("Testing failed: %s\n", e.what().c_str());
    }

    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
