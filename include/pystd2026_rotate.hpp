// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Comparator>
It rotate(It begin, It middle, It end, const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    ValueType scratch;
    const auto LEFT_SIZE = middle - begin;
    const auto RIGHT_SIZE = end - middle;

    if(LEFT_SIZE == 0) {
        return end;
    }
    if(RIGHT_SIZE == 0) {
        return begin;
    }

    const auto NUM_SWAPS = LEFT_SIZE < RIGHT_SIZE ? LEFT_SIZE : RIGHT_SIZE;
    for(ssize_t i = 0; i < NUM_SWAPS; ++i) {
        pystd2026::swap(*(begin + i), *(middle + i), scratch);
    }
    if(LEFT_SIZE < RIGHT_SIZE) {
        auto new_begin = begin + LEFT_SIZE;
        auto new_middle = new_begin + LEFT_SIZE;
        auto new_end = end;
        pystd2026::rotate(new_begin, new_middle, new_end, cmp);
    } else if(RIGHT_SIZE < LEFT_SIZE) {
        auto new_begin = begin + RIGHT_SIZE;
        auto new_middle = middle;
        auto new_end = end;
        pystd2026::rotate(new_begin, new_middle, new_end, cmp);
    }
    return begin + RIGHT_SIZE;
}

template<BasicIterator It> It rotate(It begin, It middle, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    return pystd2026::rotate(begin, middle, end, DefaultComparator<ValueType>{});
}

} // namespace pystd2026
