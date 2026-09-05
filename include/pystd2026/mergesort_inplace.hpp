// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/common.hpp>

namespace pystd2026 {

template<BasicIterator It, BasicIterator ItOut, typename Comparator>
void merge_inplace_pass(
    It left_begin, It left_end, It right_begin, It right_end, ItOut output, const Comparator &cmp) {
    while(true) {
        if(left_begin == left_end) {
            while(right_begin != right_end) {
                *output = ::pystd2026::move(*right_begin);
                ++right_begin;
                ++output;
            }
            return;
        }
        if(right_begin == right_end) {
            while(left_begin != left_end) {
                *output = ::pystd2026::move(*left_begin);
                ++left_begin;
                ++output;
            }
            return;
        }
        if(cmp.compare(*left_begin, *right_begin) > 0) {
            *output = ::pystd2026::move(*right_begin);
            ++right_begin;
            ++output;
        } else {
            *output = ::pystd2026::move(*left_begin);
            ++left_begin;
            ++output;
        }
    }
}

// If merge_block_size is, say 4, it means that the underlying data is in
// consecutive sorted blocks of size 4.
template<BasicIterator It, BasicIterator OutIt, typename Comparator>
void merge_inplace_with_block_size(
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

template<BasicIterator It1, BasicIterator It2, typename Comparator>
void merge_inplace_preprocess(It1 begin, It2 end, const Comparator &cmp, const size_t BLOCK_SIZE) {
    const auto INPUT_SIZE = end - begin;
    const size_t insertion_sort_counts = INPUT_SIZE / BLOCK_SIZE;
    for(size_t block_num = 0; block_num < insertion_sort_counts; ++block_num) {
        auto block_start = begin + block_num * BLOCK_SIZE;
        auto block_end = begin + (block_num + 1) * BLOCK_SIZE;
        if(block_end > end) {
            internal_failure("Block size computing failed.");
        }
        ::pystd2026::insertion_sort(block_start, block_end, cmp);
    }
    if(INPUT_SIZE % BLOCK_SIZE != 0) {
        ::pystd2026::insertion_sort(begin + insertion_sort_counts * BLOCK_SIZE, end, cmp);
    }
}

template<BasicIterator It, typename Comparator, size_t buffer_size = 16>
void merge_adjacent_inplace(It region_start,
                            It region_middle,
                            It region_end,
                            const Comparator &cmp) {
    if(region_start == region_middle || region_middle == region_end) {
        return;
    }

    if(region_middle < region_start) {
        abort();
    }
    if(region_end < region_middle) {
        abort();
    }
    using ValueType = ::pystd2026::remove_reference_t<decltype(*region_start)>;
    ::pystd2026::FixedVector<ValueType, buffer_size> buf;
    auto ordered_end = region_start;
    auto left_begin = region_start;
    auto left_end = region_middle;
    auto right_begin = region_middle;
    auto right_end = region_end;
    while(true) {
        buf.clear();
        while(buf.size() < buffer_size) {
            if(left_begin == left_end) {
                break;
            }
            if(right_begin == right_end) {
                break;
            }
            if(cmp.compare(*left_begin, *right_begin) >= 0) {
                buf.push_back(::pystd2026::move(*right_begin));
                ++right_begin;
            } else {
                // Potential optimization: do not move if currently at the beginning
                // of the left area.
                buf.push_back(::pystd2026::move(*left_begin));
                ++left_begin;
            }
        }
        if(left_begin == left_end || right_begin == right_end) {
            break;
        }
        const size_t num_right_taken = right_begin - left_end;
        // Make space in the beginning.
        const size_t left_items_to_move = left_end - left_begin;
        for(size_t i = 0; i < left_items_to_move; ++i) {
            *(right_begin - 1 - i) = ::pystd2026::move(*(left_end - 1 - i));
        }
        left_end += num_right_taken;
        left_begin += num_right_taken;

        for(auto &item : buf) {
            *ordered_end = ::pystd2026::move(item);
            ++ordered_end;
        }
    }

    // Final cleanup.
    if(left_begin == left_end) {
        // The buf move below is all we need.
    } else {
        if(!(right_begin == right_end)) {
            abort();
        }
        // Make space for the final shift.
        const size_t num_shifts = left_end - left_begin;
        for(size_t i = 0; i < num_shifts; ++i) {
            *(right_end - 1 - i) = ::pystd2026::move(*(left_end - 1 - i));
        }
    }
    for(auto &item : buf) {
        *ordered_end = ::pystd2026::move(item);
        ++ordered_end;
    }
}

template<BasicIterator It, typename Comparator>
void merge_inplace_sort(It begin, It end, const Comparator &cmp) {
    using ValueType = ::pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t INSERTION_SORT_LIMIT = ::pystd2026::insertion_sort_limit<ValueType>;
    const size_t INPUT_SIZE = end - begin;
    constexpr bool do_validations = false;
    if(INPUT_SIZE <= 2 * INSERTION_SORT_LIMIT) {
        ::pystd2026::insertion_sort(begin, end, cmp);
        return;
    }

    ::pystd2026::merge_inplace_preprocess(begin, end, cmp, INSERTION_SORT_LIMIT);
    size_t merge_size = INSERTION_SORT_LIMIT;

    It processed_up_to = begin;
    while(merge_size < INPUT_SIZE) {
        auto region_begin = begin;
        auto region_middle = begin;
        auto region_end = begin;
        while(true) {
            region_begin = region_end;
            if(region_begin + merge_size >= end) {
                break;
            }
            region_middle = region_begin + merge_size;
            if(region_middle + merge_size >= end) {
                region_end = end;
            } else {
                region_end = region_middle + merge_size;
            }
            ::pystd2026::merge_adjacent_inplace(region_begin, region_middle, region_end, cmp);
        }
        merge_size *= 2;
        processed_up_to = region_end;
    }
    ::pystd2026::merge_adjacent_inplace(begin, processed_up_to, end, cmp);

    if constexpr(do_validations) {
        if(!::pystd2026::is_sorted(begin, end, cmp)) {
            ::pystd2026::internal_failure("Merge sorting failed.");
        }
    }
}

template<BasicIterator It> void merge_inplace_sort(It begin, It end) {
    using ValueType = ::pystd2026::remove_reference_t<decltype(*begin)>;
    ::pystd2026::merge_inplace_sort(begin, end, DefaultComparator<ValueType>{});
}

template<WellBehaved T> void merge_inplace_sort(Span<T> array) {
    merge_inplace_sort(array.begin(), array.end());
}

template<WellBehaved T, typename Comparator>
void merge_inplace_sort(Span<T> array, const Comparator &cmp) {
    ::pystd2026::merge_inplace_sort(array.begin(), array.end(), cmp);
}

} // namespace pystd2026
