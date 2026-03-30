// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_radixsort.hpp>

// Test sorting N size_t's from a file.
// The input data is assumed to have the
// numbers 0 to N in a random order.

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

    pystd2026::radix_sort(nums.begin(), nums.end());
    for(size_t i = 0; i < NUM_ENTRIES; ++i) {
        if(nums[i] != i) {
            printf("Sort error!\n");
            return 1;
        }
    }

    return 0;
}
