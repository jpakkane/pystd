// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It> void do_bucketsort(It begin, It end, size_t round) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const ssize_t INSERTION_SORT_LIMIT = ::pystd2026::insertion_sort_limit<ValueType>;

    if(end - begin < INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
        return;
    }

    const size_t MAX_ROUNDS = sizeof(ValueType);

    if(round >= MAX_ROUNDS) {
        return;
    }

    const size_t shift = MAX_ROUNDS - round - 1;
    const ValueType MASK = 0xFF;
    const size_t NUM_BUCKETS = 256;
    ValueType counts[NUM_BUCKETS];
    size_t offsets[NUM_BUCKETS + 1];
    size_t next_free[NUM_BUCKETS + 1];

    for(size_t i = 0; i < NUM_BUCKETS; ++i) {
        counts[i] = 0;
    }

    for(auto it = begin; it != end; ++it) {
        ValueType bucket = (*it >> shift) & MASK;
        ++counts[bucket];
    }

    size_t total = 0;
    for(size_t i = 0; i < NUM_BUCKETS; ++i) {
        offsets[i] = total;
        total += counts[i];
    }
    offsets[NUM_BUCKETS] = total;
    memcpy(next_free, offsets, sizeof(offsets[0]) * (NUM_BUCKETS + 1));

    size_t current_bucket = 0;
    while(current_bucket < NUM_BUCKETS) {
        size_t current_item = next_free[current_bucket];
        if(current_item >= offsets[current_bucket + 1]) {
            ++current_bucket;
            continue;
        }

        const ValueType item_bucket = (*(begin + current_item) >> shift) & MASK;
        if(current_bucket != item_bucket) {
            const auto target_location = next_free[item_bucket];
            pystd2026::swap(*(begin + current_item), *(begin + target_location));
        }
        ++next_free[item_bucket];
    }

    // Only recursion left.
    for(size_t i = 0; i < NUM_BUCKETS; ++i) {
        do_bucketsort(begin + offsets[i], begin + offsets[i + 1], round + 1);
    }
}

template<BasicIterator It> void bucketsort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    static_assert(pystd2026::is_integral_v<ValueType>,
                  "Bucket sort only works with integral types.");
    static_assert(pystd2026::is_unsigned_v<ValueType>,
                  "Bucket sort only works with unsigned types.");

    do_bucketsort(begin, end, 0);
}

} // namespace pystd2026
