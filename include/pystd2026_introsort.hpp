// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>
#include <pystd2026_heapsort.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Comparator>
void pick_qsort_pivot_median(It begin,
                             It end,
                             const size_t NUM_MEDIAN_POINTS,
                             const Comparator &cmp) {
    const auto step = (end - begin) / NUM_MEDIAN_POINTS;
    size_t picker = step;
    for(size_t i = 1; i < NUM_MEDIAN_POINTS; ++i) {
        pystd2026::swap(*(begin + step), *(begin + i));
        picker += step;
    }

    // Ideally this should be nth_element_small, but since NUM_MEDIAN_POINTS
    // is always small, this is faster.
    pystd2026::insertion_sort(begin, begin + NUM_MEDIAN_POINTS, cmp);
}

// Converting this into a lambda inside do_introsort makes it _a lot_ slower.
inline size_t qsort_median_count(const size_t DATA_SIZE, const size_t degenerate_depth) {
    if(DATA_SIZE < 100) {
        return degenerate_depth == 0 ? 3 : 5;
    }
    if(DATA_SIZE < 1000) {
        return degenerate_depth == 0 ? 7 : 9;
    }
    if(DATA_SIZE < 10000) {
        return degenerate_depth == 0 ? 11 : 13;
    }
    if(DATA_SIZE < 100000) {
        return degenerate_depth == 0 ? 15 : 17;
    }
    return degenerate_depth == 0 ? 19 : 21;
}

template<BasicIterator It, typename Comparator>
void do_introsort(It begin,
                  It end,
                  const size_t depth,
                  const size_t degenerate_depth,
                  const size_t MAX_ROUNDS,
                  const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t num_elements = end - begin;
    const size_t degenerate_limit = num_elements / 8;

    if(num_elements <= INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end, cmp);
        return;
    }

    if(depth >= MAX_ROUNDS) {
        pystd2026::heapsort(begin, end, cmp);
        return;
    }

    const auto median_size = qsort_median_count(num_elements, degenerate_depth);
    const auto half_median = median_size / 2;

    pick_qsort_pivot_median(begin, end, median_size, cmp);

    const auto pivot_index = half_median;
    auto pivot_point = begin + pivot_index;

    // At this point we have N/2 items that are less than (or equal) to pivot
    // and N/2 that are greater than (or equal) to pivot.
    // Shrink the partitioning space by median_size - 1.
    // After this the pivot element is at the beginning of the range to be
    // partitioned.
    {
        auto from_loc = pivot_point + 1;
        auto to_loc = end - half_median - 1;
        for(size_t i = 0; i < half_median; ++i) {
            pystd2026::swap(*(from_loc + i), *(to_loc + i));
        }
    }
    auto partitioning_begin = begin + pivot_index + 1;
    auto partitioning_end = end - half_median;

    auto split_point = pystd2026::partition(
        partitioning_begin, partitioning_end, [pivot_point, &cmp](const ValueType &v) -> bool {
            return cmp.compare(v, *pivot_point) < 0;
        });
    auto last_value_point = split_point - 1;
    // After this, last_value_point is in its correct, sorted location.
    pystd2026::swap(*pivot_point, *last_value_point);

    auto left_begin = begin;
    auto left_end = last_value_point;
    auto right_begin = split_point;
    auto right_end = end;

    // Equal values adjacent to the pivot are also in their correct locations.
    // Most are in the right half as the partition critarion was <.
    // Gobble up as many as we can.
    while(right_begin < right_end && (*right_begin == *last_value_point)) {
        ++right_begin;
    }

    const size_t left_size = left_end - left_begin;
    const size_t right_size = right_end - right_begin;
    // Did we pick a bad pivot?
    // Check if we need to fall back to heapsort.
    if(left_size < degenerate_limit || right_size < degenerate_limit) {
        if(degenerate_depth > 0) {
            pystd2026::heapsort(left_begin, left_end, cmp);
            pystd2026::heapsort(right_begin, right_end, cmp);
        } else {
            do_introsort(left_begin, left_end, depth + 1, degenerate_depth + 1, MAX_ROUNDS, cmp);
            do_introsort(right_begin, right_end, depth + 1, degenerate_depth + 1, MAX_ROUNDS, cmp);
        }
    } else {
        do_introsort(left_begin, left_end, depth + 1, 0, MAX_ROUNDS, cmp);
        do_introsort(right_begin, right_end, depth + 1, 0, MAX_ROUNDS, cmp);
    }
}

template<BasicIterator It, typename Comparator>
void introsort(It begin, It end, const Comparator &cmp) {
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t num_elements = end - begin;

    if(num_elements <= INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end, cmp);
        return;
    }

    auto max_qsort_rounds = [](size_t num_elements) -> size_t {
        size_t num_rounds = 1;
        while(num_elements > 0) {
            num_elements >>= 1;
            ++num_rounds;
        }
        return num_rounds;
    };

    const size_t MAX_ROUNDS = max_qsort_rounds(num_elements);
    do_introsort(begin, end, 0, 0, MAX_ROUNDS, cmp);
}

template<BasicIterator It> void introsort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    introsort(begin, end, DefaultComparator<ValueType>{});
}

template<WellBehaved T> void introsort(pystd2026::Span<T> span) {
    introsort(span.begin(), span.end());
}

} // namespace pystd2026
