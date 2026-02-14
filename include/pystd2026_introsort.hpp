// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>
#include <pystd2026_heapsort.hpp>

namespace pystd2026 {

template<BasicIterator It, WellBehaved ValueType>
It pick_qsort_pivot(It begin, It end, ValueType &scratch) {
    auto midpoint = begin + (end - begin) / 2;
    auto last = end - 1;
    // Simple median of three.
    if(*midpoint < *begin) {
        pystd2026::swap(*begin, *midpoint, scratch);
    }
    if(*last < *midpoint) {
        pystd2026::swap(*midpoint, *last, scratch);
        if(*midpoint < *begin) {
            pystd2026::swap(*midpoint, *begin, scratch);
        }
    }
    return midpoint;
}

template<BasicIterator It, WellBehaved ValueType>
void do_introsort(
    It begin, It end, ValueType &scratch, const size_t depth, const size_t MAX_ROUNDS) {
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t num_elements = end - begin;
    const size_t degenerate_limit = num_elements / 8;

    if(num_elements <= INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
        return;
    }

    if(depth >= MAX_ROUNDS) {
        pystd2026::heapsort(begin, end);
        return;
    }

    auto pivot_point = pick_qsort_pivot(begin, end, scratch);

    // Move pivot element outside the area to be partitioned
    // so that partition operations do not move it in memory.
    if(pivot_point != begin) {
        pystd2026::swap(*pivot_point, *begin, scratch);
    }
    auto begin_after_pivot = begin + 1;

    auto split_point = pystd2026::partition(
        begin_after_pivot, end, [begin](const ValueType &v) -> bool { return v < *begin; });
    auto last_value_point = split_point - 1;
    // After this, last_value_point is in its correct, sorted location.
    pystd2026::swap(*begin, *last_value_point, scratch);

    auto left_begin = begin;
    auto left_end = last_value_point;
    auto right_begin = split_point;
    auto right_end = end;
    const size_t left_size = left_end - left_begin;
    const size_t right_size = right_end - right_begin;
    // Did we pick a bad pivot?
    // If yes, fall back to heap sort.
    // FIXME to do something smarter.
    if(left_size < degenerate_limit || right_size < degenerate_limit) {
        pystd2026::heapsort(left_begin, left_end);
        pystd2026::heapsort(right_begin, right_end);
    } else {
        do_introsort(left_begin, left_end, scratch, depth + 1, MAX_ROUNDS);
        do_introsort(right_begin, right_end, scratch, depth + 1, MAX_ROUNDS);
    }
}

template<BasicIterator It> void introsort(It begin, It end) {
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t num_elements = end - begin;

    if(num_elements <= INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
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
    do_introsort(begin, end, scratch, 0, MAX_ROUNDS);
}

template<typename T> void introsort(pystd2026::Span<T> span) {
    introsort(span.begin(), span.end());
}

} // namespace pystd2026
