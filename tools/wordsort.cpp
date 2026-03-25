// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <stdio.h>
#include <pystd2026_introsort.hpp>
#include <assert.h>

int file_main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <infile>\n", argv[0]);
        return 0;
    }
    try {
        pystd2026::Optional<pystd2026::MMapping> mmap_o = pystd2026::mmap_file(argv[1]);
        pystd2026::Vector<pystd2026::CString> words;
        if(!mmap_o) {
            printf("Could not open input file.\n");
            return 1;
        }
        auto bytes = mmap_o->view();
        pystd2026::CStringView file_view(bytes);

        //words.reserve(30000);
        auto adder = [] (const pystd2026::CStringView &piece, void *ctx) -> bool {
            auto *words = reinterpret_cast<pystd2026::Vector<pystd2026::CString>*>(ctx);
            words->emplace_back(piece);
            return true;
        };
        file_view.split(adder, &words);
        pystd2026::introsort(words.begin(), words.end());
        for(const auto &w : words) {
            printf("%s\n", w.c_str());
        }
    } catch(const pystd2026::PyException &e) {
        printf("%s\n", e.what().c_str());
        return 1;
    }

    return 0;
}
int main(int argc, char **argv) { return file_main(argc, argv); }
