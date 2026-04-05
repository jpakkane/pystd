// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Comparator>
void shell_sort_single_iteration(It begin, It end, const size_t gap, const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t buf_size = end - begin;
    ValueType scratch;
    for(size_t i = gap; i < buf_size; ++i) {
        size_t j = i;
        scratch = pystd2026::move(*(begin + i));
        for(; (j >= gap) && (*(begin + j - gap) > scratch); j -= gap) {
            *(begin + j) = pystd2026::move(*(begin + j - gap));
        }
        *(begin + j) = pystd2026::move(scratch);
    }
}

template<BasicIterator It, typename Comparator>
void shell_sort(It begin, It end, const Comparator &cmp) {
    const size_t buf_size = end - begin;

    if(buf_size <= 5) {
        pystd2026::insertion_sort(begin, end, cmp);
        return;
    }
    if(buf_size < 10 + 1)
        goto shsort4;
    if(buf_size < 23 + 1)
        goto shsort10;
    if(buf_size < 57 + 1)
        goto shsort23;
    if(buf_size < 132 + 1)
        goto shsort57;
    if(buf_size < 301 + 1)
        goto shsort132;
    if(buf_size < 701 + 1)
        goto shsort301;
    if(buf_size > 701 * 2.25) {
        size_t biggap = 701 * 2.25;
        while(biggap < buf_size) {
            biggap = 2.25 * biggap;
        }
        biggap = biggap / 2.25;
        do {
            pystd2026::shell_sort_single_iteration(begin, end, biggap, cmp);
            biggap = biggap / 2.25;
        } while(biggap > 800);
    }

    pystd2026::shell_sort_single_iteration(begin, end, 701, cmp);
shsort301:
    pystd2026::shell_sort_single_iteration(begin, end, 301, cmp);
shsort132:
    pystd2026::shell_sort_single_iteration(begin, end, 132, cmp);
shsort57:
    pystd2026::shell_sort_single_iteration(begin, end, 57, cmp);
shsort23:
    pystd2026::shell_sort_single_iteration(begin, end, 23, cmp);
shsort10:
    pystd2026::shell_sort_single_iteration(begin, end, 10, cmp);
shsort4:
    pystd2026::shell_sort_single_iteration(begin, end, 4, cmp);

    auto smallest = pystd2026::min_element(begin, begin + 4, cmp);
    pystd2026::swap(*smallest, *begin);
    pystd2026::insertion_sort_has_sentinel(begin, end, cmp);
}

template<BasicIterator It> void shell_sort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    shell_sort(begin, end, DefaultComparator<ValueType>{});
}

} // namespace pystd2026
