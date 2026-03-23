// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <stdio.h>
#include <pystd2026_hashtable.hpp>
#include <pystd2026_introsort.hpp>
#include <assert.h>

struct WordCount {
    pystd2026::U8StringView word;
    size_t count;

    int operator<=>(const WordCount &o) const {
        auto diff = (int64_t)o.count - (int64_t)count;
        if(diff != 0) {
            return diff;
        }
        return (int64_t)o.word.size_bytes() - (int64_t)word.size_bytes();
    }

    bool operator==(const WordCount &o) const { return (*this <=> o) == 0; }
};

int file_main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <infile>\n", argv[0]);
        return 0;
    }
    try {
        pystd2026::HashMap<pystd2026::U8StringView, size_t> counts;
        pystd2026::Optional<pystd2026::MMapping> mmap_o = pystd2026::mmap_file(argv[1]);
        if(!mmap_o) {
            printf("Could not open input file.\n");
            return 1;
        }
        auto &mmap = *mmap_o;
        auto bytes = mmap.view();
        auto file_as_u8 = pystd2026::U8StringView(bytes.data(), bytes.size_bytes());

        for(const auto &word : pystd2026::Loopsume(file_as_u8.split_ascii())) {
            ++counts[word];
        }
        pystd2026::Vector<WordCount> stats;
        stats.reserve(counts.size());
        for(const auto &item : counts) {
            stats.push_back(WordCount{*item.key, *item.value});
        }
        pystd2026::introsort(stats.begin(), stats.end());
        for(const auto &i : stats) {
            printf("%d %.*s\n", (int)i.count, (int)i.word.size_bytes(), i.word.data());
        }
    } catch(const pystd2026::PyException &e) {
        printf("%s\n", e.what().c_str());
        return 1;
    }

    return 0;
}
int main(int argc, char **argv) { return file_main(argc, argv); }
