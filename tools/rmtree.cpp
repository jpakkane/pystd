// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/shutil.hpp>

int main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <file or dir to remove>\n", argv[0]);
        return 1;
    }
    try {
        int num_removed = pystd2026::shutil_rmtree(argv[1]);
        printf("Number of failed removals: %d\n", num_removed);
        return num_removed;
    } catch(const pystd2026::PyException &e) {
        fprintf(stderr, "Failure: %s\n", e.what().c_str());
        return 1;
    }
}
