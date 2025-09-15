// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <assert.h>

int actual_main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <glob pattern>\n", argv[0]);
        return 1;
    }
    pystd2025::Path curdir;
    printf("Glob matches:\n\n");
    for(const auto &p : pystd2025::Loopsume(curdir.glob(argv[1]))) {
        printf("%s\n", p.c_str());
    }
    return 0;
}

int main(int argc, char **argv) {
    try {
        actual_main(argc, argv);
    } catch(const char *msg) {
        printf("%s\n", msg);
    } catch(const pystd2025::PyException &exp) {
        printf("%s\n", exp.what().c_str());
    }
}
