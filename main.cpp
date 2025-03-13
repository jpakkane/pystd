// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdio.h>
#include <pystd.hpp>

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <infile>\n", argv[0]);
        return 0;
    }
    printf("Contents of file %s:\n\n", argv[0]);
    pystd::File f(argv[1], "r");
    while(true) {
        pystd::Bytes line = f.readline_bytes();
        if(line.empty()) {
            break;
        }
        printf("%s", line.data());
    }
    return 0;
}
