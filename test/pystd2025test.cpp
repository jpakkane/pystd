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

int test_cstring_strip() {
    pystd2025::CString input(" abc \t\n\r ");
    const pystd2025::CString correct("abc");
    pystd2025::CString all_whitespace(" ");

    ASSERT(input != correct);
    input.strip();
    ASSERT(input == correct);
    ASSERT(strlen(input.data()) == 3);

    all_whitespace.strip();
    ASSERT(all_whitespace.size() == 0);
    ASSERT(strlen(all_whitespace.data()) == 0);
    return 0;
}

int test_cstring_split() {
    pystd2025::CString source("  aa bb cc  ");
    auto parts = source.split();
    ASSERT(parts.size() == 3);
    ASSERT(parts[0] == "aa");
    ASSERT(parts[1] == "bb");
    ASSERT(parts[2] == "cc");
    return 0;
}

int test_c_strings() {
    printf("Testing C strings.\n");
    int failing_subtests = 0;
    failing_subtests += test_cstring_strip();
    failing_subtests += test_cstring_split();
    return failing_subtests;
}

int test_u8string_simple() {
    pystd2025::U8String str("abc");
    ASSERT(str.size_bytes() == 3);
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

int test_u8_reverse_iterator() {
    pystd2025::U8String ascii("abc");
    ASSERT(ascii.size_bytes() == 3);
    auto start = ascii.crbegin();
    auto end = ascii.crend();
    ASSERT_WITH(start != end, "Iterator construction fail.");
    ASSERT(*start == 'c');
    ++start;
    ASSERT(*start == 'b');
    start++;
    ASSERT(*start == 'a');
    ASSERT(start != end);
    ++start;
    ASSERT(start == end);
    return 0;
}

int test_u8_reverse_iterator_cjk() {
    pystd2025::U8String cjk(daikatana);
    auto it = cjk.crbegin();
    auto c1 = *it;
    ASSERT(c1 == 20992);
    ++it;
    auto c2 = *it;
    ASSERT(c2 == 22823);
    ++it;
    ASSERT(it == cjk.crend());
    return 0;
}

int test_u8_split() {
    pystd2025::U8String source("  aa bb cc  ");
    auto parts = source.split_ascii();
    ASSERT(parts.size() == 3);
    ASSERT(parts[0] == "aa");
    ASSERT(parts[1] == "bb");
    ASSERT(parts[2] == "cc");
    return 0;
}

int test_u8_strings() {
    printf("Testing U8strings.\n");
    int failing_subtests = 0;
    failing_subtests += test_u8string_simple();
    failing_subtests += test_u8_iterator();
    failing_subtests += test_u8_iterator_cjk();
    failing_subtests += test_u8_reverse_iterator();
    failing_subtests += test_u8_reverse_iterator_cjk();
    failing_subtests += test_u8_split();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_c_strings();
        total_errors += test_u8_strings();
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
