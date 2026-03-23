// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026_rotate.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Predicate>
It stable_partition(It begin, It end, const Predicate &pred) {
    if(begin == end) {
        return begin;
    }
    // Skip elements that are already in the correct place.
    auto segment_begin = begin;
    auto scan_point = segment_begin;

    while(true) {
        auto middle = pystd2026::find_if(scan_point, end, pred);
        if(middle == end) {
            return segment_begin;
        }
        auto segment_end = pystd2026::find_if_not(middle, end, pred);
        auto rotate_point = pystd2026::rotate(segment_begin, middle, segment_end);
        if(segment_end == end) {
            return rotate_point;
        }
        segment_begin = rotate_point;
        scan_point = segment_end;
    }
}

template<BasicIterator It, WellBehaved ValueType, typename Predicate>
It stable_partition(pystd2026::Span<ValueType> span, const Predicate &pred) {
    return stable_partition(span.begin(), span.end(), pred);
}

} // namespace pystd2026
