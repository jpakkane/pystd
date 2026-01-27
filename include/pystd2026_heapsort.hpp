// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<WellBehaved It> struct HeapInfo {
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

template<WellBehaved It> void heapify(const HeapInfo<It> &hi, It loc) {
    pystd2026::Optional<It> left = hi.left_child(loc);
    auto right = hi.right_child(loc);
    It largest = loc;
    if(left && *left > *loc) {
        largest = left.value();
    }
    if(right && *right > *largest) {
        largest = right.value();
    }

    if(largest != loc) {
        pystd2026::swap(*largest, *loc);
        heapify(hi, largest);
    }
}

template<WellBehaved It> void build_heap(const HeapInfo<It> &hi) {
    int64_t i = hi.size() / 2 - 1;
    for(; i >= 0; --i) {
        heapify(hi, hi.begin + i);
    }
}

template<WellBehaved It> void heapsort(It begin, It end) {
    const size_t MIN_SIZE = 16;
    const HeapInfo original_heap{begin, end};
    if(original_heap.size() <= MIN_SIZE) {
        pystd2026::insertion_sort(begin, end);
    }

    build_heap(original_heap);
    // original_heap.validate_heap();
    for(size_t i = original_heap.size() - 1; i > MIN_SIZE; --i) {
        pystd2026::swap(*original_heap.begin, *(original_heap.begin + i));
        HeapInfo<It> shrunk_heap{begin, begin + i};
        heapify(shrunk_heap, begin);
    }
    pystd2026::insertion_sort(begin, begin + MIN_SIZE + 1);
}

} // namespace pystd2026
