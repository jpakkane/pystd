// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It1, BasicIterator It2>
void merge_pass(It1 left_begin, It1 left_end, It1 right_begin, It2 right_end, It2 output) {
    while(true) {
        if(left_begin == left_end) {
            while(right_begin != right_end) {
                *output = pystd2026::move(*right_begin);
                ++right_begin;
                ++output;
            }
            return;
        }
        if(right_begin == right_end) {
            while(left_begin != left_end) {
                *output = pystd2026::move(*left_begin);
                ++left_begin;
                ++output;
            }
            return;
        }
        if(*right_begin < *left_begin) {
            *output = pystd2026::move(*right_begin);
            ++right_begin;
            ++output;
        } else {
            *output = pystd2026::move(*left_begin);
            ++left_begin;
            ++output;
        }
    }
}

template<BasicIterator It> void mergesort(It begin, It end) {
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t INPUT_SIZE = end - begin;

    if(INPUT_SIZE <= 2 * INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end);
    }

    using Value = pystd2026::remove_reference_t<decltype(*begin)>;
    pystd2026::Vector<Value> buffer;
    buffer.reserve(INPUT_SIZE);
    for(size_t i = 0; i < INPUT_SIZE; ++i) {
        buffer.push_back(Value{});
    }

    // I did not measure, but for small sets insertion sort
    // is the fastest way to go. This is also only one pass through
    // the data rather than lg2(16) = 4 passes.
    const size_t insertion_sort_counts = INPUT_SIZE / INSERTION_SORT_LIMIT;
    for(size_t block_num = 0; block_num < insertion_sort_counts; ++block_num) {
        pystd2026::insertion_sort(begin + block_num * INSERTION_SORT_LIMIT,
                                  begin + (block_num + 1) * INSERTION_SORT_LIMIT);
    }
    if(INPUT_SIZE % INSERTION_SORT_LIMIT != 0) {
        pystd2026::insertion_sort(begin + insertion_sort_counts * INSERTION_SORT_LIMIT, end);
    }

    size_t merge_size = INSERTION_SORT_LIMIT;
    while(merge_size < INPUT_SIZE) {
        for(size_t block_start = 0; block_start < INPUT_SIZE; block_start += 2 * merge_size) {
            auto output_location = buffer.begin() + block_start;
            auto left_begin = begin + block_start;
            if(block_start + merge_size >= INPUT_SIZE) {
                // There is no right block.
                auto left_size = INPUT_SIZE - block_start;
                auto left_end = left_begin + left_size;
                merge_pass(left_begin, left_end, left_begin, left_begin, output_location);
            } else {
                auto left_end = left_begin + merge_size;
                auto right_begin = left_end;
                const auto right_block_size = block_start + 2 * merge_size >= INPUT_SIZE
                                                  ? INPUT_SIZE - (block_start + merge_size)
                                                  : merge_size;
                auto right_end = right_begin + right_block_size;
                merge_pass(left_begin, left_end, right_begin, right_end, output_location);
            }
        }
        // FIXME
        auto moveback_out = begin;
        auto move_start = buffer.begin();
        auto move_end = buffer.end();
        while(move_start != move_end) {
            *moveback_out = pystd2026::move(*move_start);
            ++moveback_out;
            ++move_start;
        }
        merge_size *= 2;
    }
}

} // namespace pystd2026
