// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It, BasicIterator ItOut, typename Comparator>
void merge_pass(
    It left_begin, It left_end, It right_begin, It right_end, ItOut output, const Comparator &cmp) {
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
        if(cmp.compare(*left_begin, *right_begin) > 0) {
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

// If merge_block_size is, say 4, it means that the underlying data is in
// consecutive sorted blocks of size 4.
template<BasicIterator It, BasicIterator OutIt, typename Comparator>
void merge_with_block_size(
    It begin, It end, OutIt output_start, size_t merge_block_size, const Comparator &cmp) {
    const size_t INPUT_SIZE = end - begin;
    for(size_t block_start = 0; block_start < INPUT_SIZE; block_start += 2 * merge_block_size) {
        auto output_location = output_start + block_start;
        auto left_begin = begin + block_start;
        if(block_start + merge_block_size >= INPUT_SIZE) {
            // There is no right block.
            auto left_size = INPUT_SIZE - block_start;
            auto left_end = left_begin + left_size;
            merge_pass(left_begin, left_end, left_begin, left_begin, output_location, cmp);
        } else {
            auto left_end = left_begin + merge_block_size;
            auto right_begin = left_end;
            const auto right_block_size = block_start + 2 * merge_block_size >= INPUT_SIZE
                                              ? INPUT_SIZE - (block_start + merge_block_size)
                                              : merge_block_size;
            auto right_end = right_begin + right_block_size;
            merge_pass(left_begin, left_end, right_begin, right_end, output_location, cmp);
        }
    }
}

template<BasicIterator It, typename Comparator>
void mergesort(It begin, It end, const Comparator &cmp) {
    const size_t INSERTION_SORT_LIMIT = 16;
    const size_t INPUT_SIZE = end - begin;
    const size_t BUFFER_SIZE = (INPUT_SIZE + 1) / 2;
    const size_t RIGHT_SIZE = BUFFER_SIZE;
    const size_t LEFT_SIZE = INPUT_SIZE - RIGHT_SIZE;
    constexpr bool do_validations = true;

    if(!(LEFT_SIZE == RIGHT_SIZE || (LEFT_SIZE + 1) == RIGHT_SIZE)) {
        internal_failure("Array computation fail.");
    }

    if(INPUT_SIZE <= 2 * INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end, cmp);
    }

    using Value = pystd2026::remove_reference_t<decltype(*begin)>;
    pystd2026::Vector<Value> buffer;
    buffer.reserve(BUFFER_SIZE);
    for(size_t i = 0; i < BUFFER_SIZE; ++i) {
        buffer.push_back(Value{});
    }
#if 0
    // I did not measure, but for small sets insertion sort is usually
    // the fastest way to go. This is also does only one pass through
    // the data rather than lg2(16) = 4 passes.
    const size_t insertion_sort_counts = INPUT_SIZE / INSERTION_SORT_LIMIT;
    for(size_t block_num = 0; block_num < insertion_sort_counts; ++block_num) {
        pystd2026::insertion_sort(begin + block_num * INSERTION_SORT_LIMIT,
                                  begin + (block_num + 1) * INSERTION_SORT_LIMIT,
                                  cmp);
    }
    if(INPUT_SIZE % INSERTION_SORT_LIMIT != 0) {
        pystd2026::insertion_sort(begin + insertion_sort_counts * INSERTION_SORT_LIMIT, end, cmp);
    }

    size_t merge_size = INSERTION_SORT_LIMIT;
#else
    size_t merge_size = 1;
#endif
    const auto left_begin = begin;
    const auto left_end = left_begin + LEFT_SIZE;
    const auto right_begin = left_end;
    const auto right_end = begin + INPUT_SIZE;

    const auto buffer_begin = buffer.begin();
    const auto buffer_end = buffer.end();

    // If the number of data points is odd, we need to store the
    // temporary data in the buffer as "right aligned", otherwise
    // merging to the left half might overflow to the right half.
    const auto right_as_buffer_begin = LEFT_SIZE == RIGHT_SIZE ? right_begin : right_begin + 1;

    bool need_fixup_move = false;
    while(true) {
        // Split in two.
        if(merge_size < BUFFER_SIZE) {
            // Right part to scratch
            merge_with_block_size(right_begin, right_end, buffer_begin, merge_size, cmp);
            // buffer_end = buffer_begin + (right_end - right_begin);
            //  Left part to right
            merge_with_block_size(left_begin, left_end, right_as_buffer_begin, merge_size, cmp);
        } else {
            need_fixup_move = true;
            break;
        }
        merge_size *= 2;
        // Join anew.
        if(merge_size < BUFFER_SIZE) {
            // Right part to left
            merge_with_block_size(right_as_buffer_begin, right_end, left_begin, merge_size, cmp);
            // Buffer part to right
            merge_with_block_size(buffer_begin, buffer_end, right_begin, merge_size, cmp);
        } else {
            // Data is already in the correct place for the final step.
            need_fixup_move = false;
            break;
        }
        merge_size *= 2;
    }

    size_t active_buffer_size = (size_t)-1;
    if(need_fixup_move) {
        active_buffer_size = LEFT_SIZE;
        auto src = begin;
        auto dst = buffer.begin();
        for(size_t i = 0; i < active_buffer_size; ++i) {
            *dst = pystd2026::move(*src);
            ++dst;
            ++src;
        }
    } else {
        active_buffer_size = RIGHT_SIZE;
    }

    if constexpr(do_validations) {
        if(!pystd2026::is_sorted(right_as_buffer_begin, right_end, cmp)) {
            internal_failure("Right buffer is not sorted before final step.");
        }
        if(!pystd2026::is_sorted(buffer_begin, buffer_begin + active_buffer_size, cmp)) {
            internal_failure("Scratch buffer is not sorted before final step.");
        }
    }
    merge_pass(right_as_buffer_begin,
               right_end,
               buffer_begin,
               buffer_begin + active_buffer_size,
               begin,
               cmp);
    if constexpr(do_validations) {
        if(!pystd2026::is_sorted(begin, end, cmp)) {
            internal_failure("Merge sorting failed.");
        }
    }
}

template<BasicIterator It> void mergesort(It begin, It end) {
    using ValueType = pystd2026::remove_const_t<decltype(*begin)>;
    mergesort(begin, end, DefaultComparator<ValueType>{});
}

template<WellBehaved T> void mergesort(Span<T> array) { mergesort(array.begin(), array.end()); }

template<WellBehaved T, typename Comparator> void mergesort(Span<T> array, const Comparator &cmp) {
    mergesort(array.begin(), array.end(), cmp);
}

} // namespace pystd2026
