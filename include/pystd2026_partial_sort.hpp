// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026_nth_element.hpp>
#include <pystd2026_introsort.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Comparator>
It partial_sort(It begin, It end, size_t nth, const Comparator &cmp) {
    auto ppoint = nth_element(begin, end, nth, cmp);
    insertion_sort(begin, ppoint, cmp);
    return ppoint;
}

template<BasicIterator It> It partial_sort(It begin, It end, size_t nth) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    return partial_sort(begin, end, nth, DefaultComparator<ValueType>{});
}

} // namespace pystd2026
