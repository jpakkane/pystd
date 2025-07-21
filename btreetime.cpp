// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <data.hpp>

#include <pystd2025_btree.hpp>

const int BTREE_NODE_SIZE = 2047;

double to_double(const timespec &t) {
    double res = t.tv_sec;
    res += t.tv_nsec / 1.0e9;
    return res;
}

void build(pystd2025::BTree<int, BTREE_NODE_SIZE> &map) {
    for(size_t i = 0; i < NUM_ENTRIES; ++i) {
        map.insert(entries[i]);
    }
}

void query(pystd2025::BTree<int, BTREE_NODE_SIZE> &map) {
    for(size_t i = 0; i < NUM_QUERIES; ++i) {
        if(!map.contains(queries[i])) {
            fprintf(stderr, "Query failure!\n");
            abort();
        }
    }
}

void measure() {
    pystd2025::BTree<int, BTREE_NODE_SIZE> map;
    timespec starttime, startquerytime, endtime;
    map.reserve(NUM_ENTRIES);
    clock_gettime(CLOCK_MONOTONIC, &starttime);
    build(map);
    clock_gettime(CLOCK_MONOTONIC, &startquerytime);
    query(map);
    clock_gettime(CLOCK_MONOTONIC, &endtime);
    const auto buildtime = to_double(startquerytime) - to_double(starttime);
    const auto querytime = to_double(endtime) - to_double(startquerytime);
    printf("Build time: %.4fs\n", buildtime);
    printf("Query time: %.4fs\n", querytime);
}

int main() {
    printf("Measuring BTree.\n");
    measure();
    // printf("\nMeasuring unordered set.\n");
    // measure<std::unordered_set<int>, true>();
}
