// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdio.h>
#include <pystd.hpp>

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <infile>\n", argv[0]);
        return 0;
    }
    try {
        pystd::Regex pattern(pystd::U8String("[a-zA-Z]+"));
        printf("First word on each line for %s:\n\n", argv[0]);
        pystd::File f(argv[1], "r");
        while(true) {
            pystd::Bytes line = f.readline_bytes();
            if(line.empty()) {
                break;
            }
            pystd::U8String u8line(line.data(), line.size());
            try {
                auto m = pystd::regex_match(pattern, u8line);
                auto m1 = m.get_submatch(0);
                printf("%s\n", m1.c_str());
            } catch(const pystd::PyException &e) {
                // No match. Go on.
                printf("\n");
            }
        }
    } catch(const pystd::PyException &e) {
        printf("%s\n", e.what().c_str());
        return 1;
    }

    return 0;
}
