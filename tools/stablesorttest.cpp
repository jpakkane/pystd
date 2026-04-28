// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_mergesort.hpp>
#include <pystd2026_indirect_sort.hpp>

// Same as sorttest, but uses stable sort.

int main() {
    FILE *f = fopen("random.dat", "rb");
    fseek(f, 0, SEEK_END);
    const auto s = ftell(f);
    fseek(f, 0, SEEK_SET);
    const size_t NUM_ENTRIES = s / sizeof(size_t);

    printf("Sorting %d numbers.\n", (int)NUM_ENTRIES);

    pystd2026::Vector<size_t> nums;
    nums.append_with_default(NUM_ENTRIES);
    const auto ignore = fread(nums.data(), sizeof(size_t), NUM_ENTRIES, f);
    (void)ignore;
    fclose(f);

    if(true) {
        pystd2026::mergesort(nums.begin(), nums.end());

        for(size_t i = 0; i < NUM_ENTRIES; ++i) {
            if(nums[i] != i) {
                printf("Sort error!\n");
                return 1;
            }
        }
    } else {
        auto indices = pystd2026::indirect_stable_sort(nums.begin(), nums.end());
        for(size_t i = 0; i < NUM_ENTRIES; ++i) {
            if(nums[i] != i) {
                printf("Sort error!\n");
                return 1;
            }
        }
    }
    return 0;
}
