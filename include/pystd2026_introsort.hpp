// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>
#include <pystd2026_heapsort.hpp>

namespace pystd2026 {

template<BasicIterator It> It pick_qsort_pivot(It begin, It end) {
    auto midpoint = begin + (end - begin) / 2;
    auto last = end - 1;
    // Simple median of three.
    if(*midpoint < *begin) {
        pystd2026::swap(*begin, *midpoint);
    }
    if(*last < *midpoint) {
        pystd2026::swap(*midpoint, *last);
        if(*midpoint < *begin) {
            pystd2026::swap(*midpoint, *begin);
        }
    }
    return midpoint;
}

template<BasicIterator It>
void do_introsort(It begin, It end, const size_t depth, const size_t MAX_ROUNDS) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t num_elements = end - begin;

    if(num_elements <= INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
        return;
    }

    if(depth >= MAX_ROUNDS) {
        pystd2026::heapsort(begin, end);
        return;
    }

    auto pivot_point = pick_qsort_pivot(begin, end);

    // FIXME, makes a copy, so might be inefficient.
    const ValueType pivot_value = *pivot_point;

    auto split_point = pystd2026::partition(
        begin, end, [&pivot_value](const ValueType &v) -> bool { return v < pivot_value; });
    auto left_begin = begin;
    auto left_end = split_point;
    auto right_begin = split_point;
    auto right_end = end;
    const auto left_size = left_end - left_begin;
    const auto right_size = right_end - right_begin;
    // Did we pick a bad pivot?
    // If yes, fall back to heap sort.
    // FIXME to do something smarter.
    if(left_size <= 1 || right_size <= 1) {
        pystd2026::heapsort(begin, end);
        return;
    }
    do_introsort(left_begin, left_end, depth + 1, MAX_ROUNDS);
    do_introsort(right_begin, right_end, depth + 1, MAX_ROUNDS);
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
    do_introsort(begin, end, 0, MAX_ROUNDS);
}

template<typename T> void introsort(pystd2026::Span<T> span) {
    introsort(span.begin(), span.end());
}

} // namespace pystd2026
