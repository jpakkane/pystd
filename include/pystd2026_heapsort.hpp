// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It> struct HeapInfo {
    It begin;
    It end;

    pystd2026::Optional<It> left_child(const It &i) const {
        const auto off_in = i - begin;
        const auto off_out = 2 * off_in + 1;
        if((size_t)off_out >= size()) {
            return {};
        }
        return begin + off_out;
    }

    pystd2026::Optional<It> right_child(const It &i) const {
        const auto off_in = i - begin;
        const auto off_out = 2 * off_in + 2;
        if((size_t)off_out >= size()) {
            return {};
        }
        return begin + off_out;
    }

    pystd2026::Optional<It> parent(const It &i) const {
        const auto off = i - begin;
        if(off == 0) {
            return {};
        }
        return begin + off / 2;
    }

    size_t size() const { return end - begin; }

    void validate_heap() const {
        for(size_t i = 0; i < size() / 2 - 1; ++i) {
            auto cur = begin + i;
            auto l = left_child(cur).value();
            auto r = right_child(cur).value();
            if(*l > *cur) {
                throw "FAIL";
            }
            if(*r > *cur) {
                throw "FAIL2";
            }
        }
    }
};

template<BasicIterator It, typename Comparator>
void heapify(const HeapInfo<It> &hi, It loc, const Comparator &cmp) {
    pystd2026::Optional<It> left = hi.left_child(loc);
    auto right = hi.right_child(loc);
    It largest = loc;
    if(left && cmp.compare(*left, *loc) > 0) {
        largest = left.value();
    }
    if(right && cmp.compare(*right, *largest) > 0) {
        largest = right.value();
    }

    if(largest != loc) {
        pystd2026::swap(*largest, *loc);
        heapify(hi, largest, cmp);
    }
}

template<BasicIterator It, typename Comparator>
void build_heap(const HeapInfo<It> &hi, const Comparator &cmp) {
    int64_t i = hi.size() / 2 - 1;
    for(; i >= 0; --i) {
        heapify(hi, hi.begin + i, cmp);
    }
}

template<BasicIterator It, typename Comparator>
void heapsort(It begin, It end, const Comparator &cmp) {
    const size_t MIN_SIZE = 16;
    if((size_t)(end - begin) <= MIN_SIZE) {
        pystd2026::insertion_sort(begin, end, cmp);
        return;
    }
    const HeapInfo original_heap{begin, end};

    build_heap(original_heap, cmp);
    // original_heap.validate_heap();
    for(size_t i = original_heap.size() - 1; i > MIN_SIZE; --i) {
        pystd2026::swap(*original_heap.begin, *(original_heap.begin + i));
        HeapInfo<It> shrunk_heap{begin, begin + i};
        heapify(shrunk_heap, begin, cmp);
    }
    pystd2026::insertion_sort(begin, begin + MIN_SIZE + 1);
}

template<BasicIterator It> void heapsort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    heapsort(begin, end, DefaultComparator<ValueType>{});
}

template<WellBehaved T> void heapsort(pystd2026::Span<T> span) {
    heapsort(span.begin(), span.end());
}

} // namespace pystd2026
