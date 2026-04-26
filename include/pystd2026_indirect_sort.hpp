// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>
#include <pystd2026_introsort.hpp>

namespace pystd2026 {

template<BasicIterator It, typename UnderlyingComparator> struct IndirectComparator {
    const It begin;
    const UnderlyingComparator &cmp;
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;

    int compare(const size_t a, const size_t b) const noexcept {
        return cmp.compare(*(begin + a), *(begin + b));
    }

    bool equal(const size_t a, const size_t b) const noexcept {
        return cmp.equal(*(begin + a), *(begin + b));
    }
};

template<BasicIterator It, typename Comparator>
pystd2026::Vector<size_t> indirect_sort(It begin, It end, const Comparator &cmp) {
    pystd2026::Vector<size_t> indices;
    const size_t data_size = end - begin;
    indices.reserve(data_size);
    for(size_t i = 0; i < data_size; ++i) {
        indices.push_back(i);
    }

    const IndirectComparator icmp{.begin = begin, .cmp = cmp};

    // Fixme, the underlying sort algorithm should be a parameter,
    // but I could not figure out how to make it work.
    pystd2026::introsort(indices.begin(), indices.end(), icmp);
    return indices;
}

template<BasicIterator It> pystd2026::Vector<size_t> indirect_sort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    return indirect_sort(begin, end, DefaultComparator<ValueType>{});
}
} // namespace pystd2026
