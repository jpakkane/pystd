// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdio.h>

int main(int argc, char **argv) {
    auto *x = new int;
    printf("Hello.\n");
    delete x;
    return 0;
}
