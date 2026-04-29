// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2026.hpp>

namespace pystd2026 {

template<WellBehaved ValueType, BasicIterator It>
void do_radix_sort(It begin, It end, const ValueType needs_processing_mask, const size_t round) {
    const ssize_t INSERTION_SORT_LIMIT = ::pystd2026::insertion_sort_limit<ValueType>;
    const auto num_bits = sizeof(ValueType) * 8;
    const ValueType sort_bit = (1 << (num_bits - 1 - (ValueType)round));

    if(end - begin < INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
        return;
    }

    if(!(needs_processing_mask & sort_bit)) {
        do_radix_sort(begin, end, needs_processing_mask, round + 1);
        return;
    }
    const auto picker = [sort_bit](const ValueType &v) -> bool { return !(v & sort_bit); };
    auto split_point = pystd2026::partition(begin, end, picker);

    if(sort_bit != 1) {
        do_radix_sort(begin, split_point, needs_processing_mask, round + 1);
        do_radix_sort(split_point, end, needs_processing_mask, round + 1);
    }
}

template<BasicIterator It> void radix_sort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    static_assert(pystd2026::is_unsigned_v<ValueType>,
                  "Radix sort currently only supports unsigned integers. Patches welcome.");
    const ssize_t INSERTION_SORT_LIMIT = ::pystd2026::insertion_sort_limit<ValueType>;

    if(end - begin <= INSERTION_SORT_LIMIT) {
        return pystd2026::insertion_sort(begin, end);
    }

    // If all bits in one location are either one or zero, that bit
    // location does not need to be processed.
    ValueType one_bits = 0;
    ValueType zero_bits = 0;
    for(auto it = begin; it != end; ++it) {
        one_bits |= *it;
        zero_bits |= ~(*it);
    }
    const ValueType needs_processing = one_bits & zero_bits;

    do_radix_sort(begin, end, needs_processing, 0);
}

template<WellBehaved ValueType> void radix_sort(pystd2026::Span<ValueType> sp) {
    radix_sort(sp.begin(), sp.end());
}

} // namespace pystd2026
