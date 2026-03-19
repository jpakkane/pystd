// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>
#include <pystd2026_heapsort.hpp>

namespace pystd2026 {

template<BasicIterator It, WellBehaved ValueType, typename Comparator>
It pick_qsort_pivot_median3(It begin, It end, ValueType &scratch, const Comparator &cmp) {
    auto midpoint = begin + (end - begin) / 2;
    auto last = end - 1;
    // Simple median of three.
    if(cmp.compare(*midpoint, *begin) < 0) {
        pystd2026::swap(*begin, *midpoint, scratch);
    }
    if(cmp.compare(*last, *midpoint) < 0) {
        pystd2026::swap(*midpoint, *last, scratch);
        if(cmp.compare(*midpoint, *begin) < 0) {
            pystd2026::swap(*midpoint, *begin, scratch);
        }
    }
    return midpoint;
}

template<BasicIterator It, WellBehaved ValueType, typename Comparator>
It pick_qsort_pivot_median5(It begin, It end, ValueType &scratch, const Comparator &cmp) {
    const auto step = (end - begin) / 5;
    auto m1 = begin + step;
    auto m2 = m1 + step;
    auto m3 = m2 + step;
    auto last = end - 1;
    pystd2026::swap(*m1, *(begin + 1), scratch);
    pystd2026::swap(*m2, *(begin + 2), scratch);
    pystd2026::swap(*m3, *(begin + 3), scratch);
    pystd2026::swap(*last, *(begin + 4), scratch);

    // Optimally nth element.
    pystd2026::insertion_sort(begin, begin + 5, cmp);
    return begin + 2;
}

template<BasicIterator It, WellBehaved ValueType, typename Comparator>
void do_introsort(It begin,
                  It end,
                  ValueType &scratch,
                  const size_t depth,
                  const size_t degenerate_depth,
                  const size_t MAX_ROUNDS,
                  const Comparator &cmp) {
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

    auto pivot_point = degenerate_depth == 0 ? pick_qsort_pivot_median3(begin, end, scratch, cmp)
                                             : pick_qsort_pivot_median5(begin, end, scratch, cmp);

    // Move pivot element outside the area to be partitioned
    // so that partition operations do not move it in memory.
    if(pivot_point != begin) {
        pystd2026::swap(*pivot_point, *begin, scratch);
    }
    auto begin_after_pivot = begin + 1;

    auto split_point =
        pystd2026::partition(begin_after_pivot, end, [begin, &cmp](const ValueType &v) -> bool {
            return cmp.compare(v, *begin) < 0;
        });
    auto last_value_point = split_point - 1;
    // After this, last_value_point is in its correct, sorted location.
    pystd2026::swap(*begin, *last_value_point, scratch);

    auto left_begin = begin;
    auto left_end = last_value_point;
    auto right_begin = split_point;
    auto right_end = end;

    // Equal values adjacent to the pivot are also in their correct locations.
    // They are all in the right half as the sort critarion was <.
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
            do_introsort(
                left_begin, left_end, scratch, depth + 1, degenerate_depth + 1, MAX_ROUNDS, cmp);
            do_introsort(
                right_begin, right_end, scratch, depth + 1, degenerate_depth + 1, MAX_ROUNDS, cmp);
        }
    } else {
        do_introsort(left_begin, left_end, scratch, depth + 1, 0, MAX_ROUNDS, cmp);
        do_introsort(right_begin, right_end, scratch, depth + 1, 0, MAX_ROUNDS, cmp);
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
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    ValueType scratch;
    do_introsort(begin, end, scratch, 0, 0, MAX_ROUNDS, cmp);
}

template<BasicIterator It> void introsort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    introsort(begin, end, DefaultComparator<ValueType>{});
}

template<WellBehaved T> void introsort(pystd2026::Span<T> span) {
    introsort(span.begin(), span.end());
}

} // namespace pystd2026
