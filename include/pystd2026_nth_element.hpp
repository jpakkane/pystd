// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026.hpp>
#include <pystd2026_heapsort.hpp>

namespace pystd2026 {

template<BasicIterator It, typename Comparator>
It nth_element_heap(It begin, It end, size_t nth, const Comparator &cmp) {
    const size_t data_size = end - begin;
    if(nth >= data_size) {
        throw PyException("Nth element out of bounds.");
    }
    if(nth == data_size + 1) {
        auto max_loc = pystd2026::max_element(begin, end, cmp);
        pystd2026::swap(*(end - 1), *max_loc);
        return end - 1;
    }
    const HeapInfo small_heap{begin, begin + nth + 1};
    build_heap(small_heap, cmp);
    for(auto it = begin + nth + 1; it < end; ++it) {
        if(cmp.compare(*it, *begin) < 0) {
            pystd2026::swap(*begin, *it);
            heapify(small_heap, begin, cmp);
        }
    }
    pystd2026::swap(*begin, *(begin + nth));
    return begin + nth;
}

template<BasicIterator It, typename Comparator>
It nth_element(It begin, It end, size_t nth, const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t INSERTION_SORT_LIMIT = ::pystd2026::insertion_sort_limit<ValueType>;
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
            if(buf_size <= 2 * INSERTION_SORT_LIMIT) {
                pystd2026::insertion_sort(begin, end, cmp);
                return begin + nth;
            } else {
                return nth_element_heap(begin, end, nth, cmp);
            }
        }
        ++round;
        size_t random_number = (size_t)&(*begin);
        random_number >>= sizeof(ValueType);
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
    return nth_element_heap(begin, end, nth, DefaultComparator<ValueType>{});
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
