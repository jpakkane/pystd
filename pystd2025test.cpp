// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <string.h>

#define ASSERT(statement, message)                                                                 \
    if((statement)) {                                                                              \
        printf("%s\n", message);                                                                   \
        return 1;                                                                                  \
    }

int test_string_simple() {
    pystd2025::U8String str("abc");
    ASSERT(strcmp("abc", str.c_str()) != 0, "String construction failed.");
    return 0;
}

int test_strings() {
    printf("Testing U8strings.\n");
    int failing_subtests = 0;
    failing_subtests += test_string_simple();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    total_errors += test_strings();
    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
