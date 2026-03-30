// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026.hpp>

namespace pystd2026 {

// This should only used for small data sizes.

template<BasicIterator It, typename Comparator>
It nth_element_small(It begin, It end, const size_t nth, const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    ValueType scratch;
    if(nth >= (size_t)(end - begin)) {
        throw PyException("Requested nth element that is out of bounds.");
    }
    It nth_point = begin + nth;
    pystd2026::insertion_sort(begin, nth_point + 1);

    for(It unhandled_point = nth_point + 1; unhandled_point != end; ++unhandled_point) {
        if(cmp.compare(*unhandled_point, *nth_point) < 0) {
            pystd2026::swap(*unhandled_point, *nth_point, scratch);
            auto current = nth_point;
            auto previous = current - 1;
            while(previous >= begin && cmp.compare(*current, *previous) < 0) {
                pystd2026::swap(*previous, *current, scratch);
                --previous;
                --current;
            }
        }
    }

    return nth_point;
}

template<BasicIterator It> It nth_element_small(It begin, It end, const size_t nth) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    return nth_element_small(begin, end, nth, DefaultComparator<ValueType>{});
}

} // namespace pystd2026
