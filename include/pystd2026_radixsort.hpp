// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It> void do_radix_sort(It begin, It end, const size_t round) {
    const ssize_t INSERTION_SORT_LIMIT = 16;
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const auto num_bits = sizeof(ValueType) * 8;
    const ValueType sort_bit = (1 << (num_bits - 1 - (ValueType)round));

    if(end - begin < INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
        return;
    }

    const auto picker = [sort_bit](const ValueType &v) -> bool { return !(v & sort_bit); };
    auto split_point = pystd2026::partition(begin, end, picker);

    if(sort_bit != 1) {
        do_radix_sort(begin, split_point, round + 1);
        do_radix_sort(split_point, end, round + 1);
    }
}

template<BasicIterator It> void radix_sort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const auto num_bits = sizeof(ValueType) * 8;
    static_assert(pystd2026::is_unsigned_v<ValueType>,
                  "Radix sort currently only supports unsigned integers. Patches welcome.");
    const ssize_t INSERTION_SORT_LIMIT = 16;
    if(end - begin <= INSERTION_SORT_LIMIT) {
        return pystd2026::insertion_sort(begin, end);
    }

    ValueType max_val = *pystd2026::max_element(begin, end, DefaultComparator<ValueType>{});
    size_t highest_bit = 0;
    while(max_val > 0) {
        max_val /= 2;
        ++highest_bit;
    }
    const auto start_at_round = num_bits - highest_bit;

    do_radix_sort(begin, end, start_at_round);
}

template<WellBehaved ValueType> void radix_sort(pystd2026::Span<ValueType> sp) {
    radix_sort(sp.begin(), sp.end());
}

} // namespace pystd2026
