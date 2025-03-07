// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdio.h>
#include <pystd.hpp>

int main(int argc, char **argv) {
    pystd::unique_ptr<int> up(new int);
    printf("Hello.\n");
    return 0;
}
