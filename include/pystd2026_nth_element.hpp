// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Comparator>
It nth_element(It begin, It end, size_t nth, const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t INSERTION_SORT_LIMIT = 16;
    ValueType scratch;
    if(nth >= (size_t)(end - begin)) {
        throw PyException("Requested nth element that is out of bounds.");
    }
    size_t round = 0;

    const auto max_rounds = [](size_t num_elements) -> size_t {
        size_t num_rounds = 1;
        while(num_elements > 0) {
            num_elements >>= 1;
            ++num_rounds;
        }
        return num_rounds;
    }(end - begin);

    while(true) {
        const size_t buf_size = end - begin;
        if(buf_size < INSERTION_SORT_LIMIT) {
            pystd2026::insertion_sort(begin, end, cmp);
            return begin + nth;
        }
        if(round >= max_rounds) {
            // FIXME: maybe should use heap sort.
            // However this should only be hit rarely.
            pystd2026::insertion_sort(begin, end, cmp);
            return begin + nth;
        }
        ++round;
        size_t random_number = (size_t)&(*begin);
        random_number >>= (sizeof(ValueType) / 8);
        size_t pivot_offset = (random_number + round) % buf_size;
        if(pivot_offset != 0) {
            pystd2026::swap(*begin, *(begin + pivot_offset), scratch);
        }
        auto split_point = pystd2026::partition(begin + 1, end, [begin, &cmp](const ValueType &v) {
            return cmp.compare(v, *begin) < 0;
        });
        // const size_t left_size = split_point - begin;
        // const size_t right_size = end - split_point;
        const size_t split_offset = split_point - begin;
        const size_t last_value_offset = split_offset - 1;
        auto last_value_point = split_point - 1;
        pystd2026::swap(*begin, *last_value_point, scratch);
        if(last_value_offset < nth) {
            nth -= split_offset;
            begin = split_point;
        } else if(last_value_offset > nth) {
            end = last_value_point;
        } else {
            return last_value_point;
        }
    }
}

template<BasicIterator It> It nth_element(It begin, It end, const size_t nth) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    return nth_element(begin, end, nth, DefaultComparator<ValueType>{});
}

template<WellBehaved ValueType, typename Comparator>
size_t nth_element(pystd2026::Span<ValueType> span, const size_t nth, const Comparator &cmp) {
    return nth_element(span.begin(), span.end(), nth, cmp) - span.begin();
}

template<WellBehaved ValueType>
size_t nth_element(pystd2026::Span<ValueType> span, const size_t nth) {
    return nth_element(span, span, nth, DefaultComparator<ValueType>{});
}

} // namespace pystd2026
