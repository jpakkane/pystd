// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdio.h>
#include <pystd.hpp>

int file_main(int argc, char **argv) {
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

int main(int argc, char **argv) {
    if(true) {
        return hashmap_main(argc, argv);
    } else {
        return file_main(argc, argv);
    }
}
