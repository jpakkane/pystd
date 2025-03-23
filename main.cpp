// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdio.h>
#include <pystd.hpp>
#include <assert.h>

struct WordCount {
    pystd::U8String *str;
    size_t count;

    int operator<=>(const WordCount &o) const { return o.count - o.count; }
};

int file_main(int argc, char **argv) {
    if(argc != 2) {
        printf("%s <infile>\n", argv[0]);
        return 0;
    }
    pystd::HashMap<pystd::U8String, size_t> counts;
    try {
        pystd::File f(argv[1], "r");
        for(auto &&line : f) {
            pystd::U8String u8line(line.data(), line.size());
            assert(line.size() == u8line.size_bytes());
            auto words = u8line.split();
            for(const auto &w : words) {
                auto *c = counts.lookup(w);
                if(c) {
                    ++(*c);
                } else {
                    counts.insert(w, 1);
                }
            }
        }
        printf("%d unique words.\n", (int)counts.size());
    } catch(const pystd::PyException &e) {
        printf("%s\n", e.what().c_str());
        return 1;
    }

    return 0;
}

int hashmap_main(int, char **) {
    pystd::HashMap<pystd::U8String, size_t> wordcounter;
    pystd::U8String key1("key1");
    pystd::U8String key2("key2");

    printf("Initial size: %d\n", (int)wordcounter.size());
    printf("Contains key1: %d\n", (int)wordcounter.contains(key1));
    printf("Contains key2: %d\n", (int)wordcounter.contains(key2));

    printf("Inserting key1.\n");
    wordcounter.insert(key1, 66);
    printf("Size: %d\n", (int)wordcounter.size());
    printf("Contains key1: %d\n", (int)wordcounter.contains(key1));
    printf("Value of key1: %d\n", (int)*wordcounter.lookup(key1));
    printf("Contains key2: %d\n", (int)wordcounter.contains(key2));

    return 0;
}

int split_main(int, char **) {
    pystd::U8String text("aa bb cc");
    auto r = text.split();
    printf("Split array size: %d\n", (int)r.size());
    for(size_t i = 0; i < r.size(); ++i) {
        printf(" %s\n", r[i].c_str());
    }
    return 0;
}

int main(int argc, char **argv) {
    if(false) {
        return hashmap_main(argc, argv);
    } else if(false) {
        return split_main(argc, argv);
    } else {
        return file_main(argc, argv);
    }
}
