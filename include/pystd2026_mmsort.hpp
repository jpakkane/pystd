// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<WellBehaved Value> struct MMQueueItem {
    Value value;
    unsigned char source;
};

template<BasicIterator It> struct BlockInfo {
    size_t size;
    It data;
};

// Smallest value has preference.
template<typename T> struct PriorityQueue {
    template<typename Comparator> void initialize(const Comparator &cmp) {
        pystd2026::insertion_sort(backing.begin(), backing.end(), cmp);
    }

    template<typename Comparator> void front_changed(const Comparator &cmp) {
        if(cmp.compare(backing[0], backing[1]) >= 1) {
            pystd2026::swap(backing[0], backing[1]);
            size_t current = 1;
            size_t next = 2;
            while(next < backing.size() && cmp.compare(backing[current], backing[next]) > 0) {
                pystd2026::swap(backing[current], backing[next]);
                ++current;
                ++next;
            }
        }
    }

    bool is_empty() const { return backing.is_empty(); }

    size_t size() const { return backing.size(); }

    auto front() const { return backing.front(); }

    template<typename T2, typename Comparator> void replace_front(T2 &&new_value, Comparator cmp) {
        backing.front().value = pystd2026::move(new_value);
        front_changed(cmp);
    }

    void pop_front() {
        pystd2026::rotate_first_to_back(backing.begin(), backing.end());
        backing.pop_back();
    }

    T backing;
};

template<WellBehaved ValueType, typename ValueComparator> struct QueueComparator {
    const ValueComparator *cmp = nullptr;

    int compare(const MMQueueItem<ValueType> &a, const MMQueueItem<ValueType> &b) const noexcept {
        auto v = cmp->compare(a.value, b.value);
        if(v != 0) {
            return v;
        }
        if(a.source < b.source) {
            return -1;
        }
        if(a.source > b.source) {
            return 1;
        }
        return 0;
    }

    bool equal(const MMQueueItem<ValueType> &a, const MMQueueItem<ValueType> &b) const noexcept {
        return a.source == b.source && cmp->equal(a.value, b.value);
    }
};

template<BasicIterator It> void mm_debug_printf(const It begin, const It end, const char *name) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    if constexpr(pystd2026::is_integral_v<ValueType>) {
        printf("%s\n", name);
        for(auto it = begin; it != end; ++it) {
            printf("%d ", (int)*it);
        }
        printf("\n\n");
    }
}

template<size_t QUEUE_SIZE, WellBehaved ValueType, typename Queue, BasicIterator It>
void setup_queue(BlockInfo<It> *binfo,
                 Queue &queue,
                 It begin,
                 const size_t data_set_size,
                 const size_t block_size) {
    static_assert(QUEUE_SIZE > 0);
    size_t i = 0;
    const auto queue_offset = queue.size();
    const size_t num_full_blocks = data_set_size / block_size;
    for(; i < num_full_blocks; ++i) {
        binfo[i] = BlockInfo{block_size, begin + i * block_size};
    }
    if((data_set_size % block_size) != 0) {
        binfo[i] = BlockInfo{data_set_size % block_size, begin + i * block_size};
        ++i;
    }

    for(size_t j = 0; j < i; ++j) {
        queue.emplace_back(pystd2026::move(*binfo[j].data), (unsigned char)(j + queue_offset));
        --(binfo[j].size);
        ++(binfo[j].data);
    }
}

template<size_t QUEUE_SIZE, BasicIterator It, BasicIterator ItOut, typename Comparator>
void mm_merge_pass(It left_begin,
                   It left_end,
                   It right_begin,
                   It right_end,
                   ItOut output,
                   size_t block_size,
                   const Comparator &value_cmp) {

    using ValueType = pystd2026::remove_reference_t<decltype(*left_begin)>;

    PriorityQueue<pystd2026::FixedVector<MMQueueItem<ValueType>, QUEUE_SIZE>> queue;
    QueueComparator<ValueType, Comparator> cmp(&value_cmp);
    BlockInfo<It> binfo[QUEUE_SIZE];
    const size_t left_size = left_end - left_begin;
    const size_t right_size = right_end - right_begin;

    static_assert(QUEUE_SIZE > 0);
    static_assert(QUEUE_SIZE % 2 == 0, "Queue size must be an even number");

    if(right_size == 0) {
        setup_queue<QUEUE_SIZE, ValueType>(
            binfo, queue.backing, left_begin, left_end - left_begin, block_size);
    } else {
        setup_queue<QUEUE_SIZE / 2, ValueType>(
            binfo, queue.backing, left_begin, left_size, block_size);
        setup_queue<QUEUE_SIZE / 2, ValueType>(
            binfo + queue.size(), queue.backing, right_begin, right_size, block_size);
    }
    queue.initialize(cmp);

    if(queue.is_empty()) {
        return;
    }
    while(true) {
        const auto source_block = queue.front().source;
        *output = pystd2026::move(queue.front().value);
        ++output;
        if(queue.size() == 1) {
            // Move remaining data.
            auto source = binfo[source_block].data;
            const auto remaining = binfo[source_block].size;
            for(size_t i = 0; i < remaining; ++i) {
                *output = pystd2026::move(*source);
                ++output;
                ++source;
            }
            return;
        } else {
            if(binfo[source_block].size == 0) {
                // Block exhausted, remove it from the queue.
                queue.pop_front();
            } else {
                queue.replace_front(pystd2026::move(*binfo[source_block].data), cmp);
                --(binfo[source_block].size);
                ++(binfo[source_block].data);
            }
        }
    }
}

// If merge_block_size is, say 4, it means that the underlying data is in
// consecutive sorted blocks of size 4.
template<size_t QUEUE_SIZE, BasicIterator It, BasicIterator OutIt, typename Comparator>
void mm_merge_with_block_size(
    It begin, It end, OutIt output_start, size_t merge_block_size, const Comparator &value_cmp) {

    const size_t INPUT_SIZE = end - begin;
    const auto MERGED_SIZE = QUEUE_SIZE * merge_block_size;
    for(size_t block_start = 0; block_start < INPUT_SIZE; block_start += MERGED_SIZE) {
        auto output_location = output_start + block_start;
        auto workblock_begin = begin + block_start;
        size_t actual_size;
        It actual_end;
        if(block_start + MERGED_SIZE >= INPUT_SIZE) {
            actual_size = INPUT_SIZE - block_start;
        } else {
            actual_size = MERGED_SIZE;
        }
        actual_end = workblock_begin + actual_size;

        mm_merge_pass<QUEUE_SIZE>(workblock_begin,
                                  actual_end,
                                  actual_end,
                                  actual_end,
                                  output_location,
                                  merge_block_size,
                                  value_cmp);
    }
}

template<BasicIterator It1, BasicIterator It2, typename Comparator>
void mm_merge_preprocess(It1 begin, It2 end, const Comparator &cmp, const size_t BLOCK_SIZE) {
    const auto INPUT_SIZE = end - begin;
    const size_t insertion_sort_counts = INPUT_SIZE / BLOCK_SIZE;
    for(size_t block_num = 0; block_num < insertion_sort_counts; ++block_num) {
        auto block_start = begin + block_num * BLOCK_SIZE;
        auto block_end = begin + (block_num + 1) * BLOCK_SIZE;
        if(block_end > end) {
            internal_failure("Block size computing failed.");
        }
        pystd2026::insertion_sort(block_start, block_end, cmp);
    }
    if(INPUT_SIZE % BLOCK_SIZE != 0) {
        pystd2026::insertion_sort(begin + insertion_sort_counts * BLOCK_SIZE, end, cmp);
    }
}

template<size_t QUEUE_SIZE, BasicIterator It, typename Comparator>
void mmsort(It begin, It end, const Comparator &cmp) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    const size_t INSERTION_SORT_LIMIT = ::pystd2026::insertion_sort_limit<ValueType>;
    const size_t INPUT_SIZE = end - begin;
    const size_t BUFFER_SIZE = (INPUT_SIZE + 1) / 2;
    const size_t RIGHT_SIZE = BUFFER_SIZE;
    const size_t LEFT_SIZE = INPUT_SIZE - RIGHT_SIZE;
    constexpr bool do_debug_prints = false;

    if(!(LEFT_SIZE == RIGHT_SIZE || (LEFT_SIZE + 1) == RIGHT_SIZE)) {
        internal_failure("Array computation fail.");
    }

    if(INPUT_SIZE <= 2 * INSERTION_SORT_LIMIT) {
        pystd2026::insertion_sort(begin, end, cmp);
        return;
    }

    pystd2026::Vector<ValueType> buffer;
    buffer.reserve(BUFFER_SIZE);
    buffer.append_with_default(BUFFER_SIZE);

    const auto left_begin = begin;
    const auto left_end = left_begin + LEFT_SIZE;
    const auto right_begin = left_end;
    const auto right_end = begin + INPUT_SIZE;

    const auto buffer_begin = buffer.begin();
    const auto buffer_end = buffer.end();

    mm_merge_preprocess(left_begin, left_end, cmp, INSERTION_SORT_LIMIT);
    mm_merge_preprocess(right_begin, right_end, cmp, INSERTION_SORT_LIMIT);
    size_t merge_size = INSERTION_SORT_LIMIT;

    // If the number of data points is odd, we need to store the
    // temporary data in the buffer as "right aligned", otherwise
    // merging to the left half might overflow to the right half.
    const auto right_as_buffer_begin = LEFT_SIZE == RIGHT_SIZE ? right_begin : right_begin + 1;

    bool need_fixup_move = false;
    while(true) {
        if constexpr(do_debug_prints) {
            mm_debug_printf(begin, end, "Array");
        }
        // Split in two.
        if(merge_size < BUFFER_SIZE) {
            // Right part to scratch
            mm_merge_with_block_size<QUEUE_SIZE>(
                right_begin, right_end, buffer_begin, merge_size, cmp);
            // buffer_end = buffer_begin + (right_end - right_begin);
            //  Left part to right
            mm_merge_with_block_size<QUEUE_SIZE>(
                left_begin, left_end, right_as_buffer_begin, merge_size, cmp);
        } else {
            need_fixup_move = true;
            break;
        }
        merge_size *= QUEUE_SIZE;
        // Join anew.
        if(merge_size < BUFFER_SIZE) {
            // Right part to left
            mm_merge_with_block_size<QUEUE_SIZE>(
                right_as_buffer_begin, right_end, left_begin, merge_size, cmp);
            // Buffer part to right
            mm_merge_with_block_size<QUEUE_SIZE>(
                buffer_begin, buffer_end, right_begin, merge_size, cmp);
        } else {
            // Data is already in the correct place for the final step.
            need_fixup_move = false;
            break;
        }
        merge_size *= QUEUE_SIZE;
    }

    size_t active_buffer_size = (size_t)-1;
    if(need_fixup_move) {
        if constexpr(do_debug_prints) {
            mm_debug_printf(begin, end, "Before fixup move.");
        }
        active_buffer_size = LEFT_SIZE;
        auto src = begin;
        auto dst = buffer_begin;
        for(size_t i = 0; i < active_buffer_size; ++i) {
            *dst = pystd2026::move(*src);
            ++dst;
            ++src;
        }
        if constexpr(do_debug_prints) {
            mm_debug_printf(
                buffer_begin, buffer_begin + active_buffer_size, "Buffer after fixup move.");
            mm_debug_printf(right_begin, right_begin + RIGHT_SIZE, "Right after fixup move.");
        }
    } else {
        active_buffer_size = RIGHT_SIZE;
    }

    // In order to keep equal values in the same order,
    // we need to do the merge in different orders.
    if(need_fixup_move) {
        mm_merge_pass<QUEUE_SIZE>(buffer_begin,
                                  buffer_begin + active_buffer_size,
                                  right_begin,
                                  right_end,
                                  begin,
                                  merge_size,
                                  cmp);
    } else {
        mm_merge_pass<QUEUE_SIZE>(right_as_buffer_begin,
                                  right_end,
                                  buffer_begin,
                                  buffer_begin + active_buffer_size,
                                  begin,
                                  merge_size,
                                  cmp);
    }
}

template<BasicIterator It> void mmsort(It begin, It end) {
    using ValueType = pystd2026::remove_reference_t<decltype(*begin)>;
    mmsort<8, It, DefaultComparator<ValueType>>(begin, end, DefaultComparator<ValueType>{});
}

template<WellBehaved T> void mmsort(Span<T> array) { memort(array.begin(), array.end()); }

template<WellBehaved T, typename Comparator> void mmsort(Span<T> array, const Comparator &cmp) {
    mmsort(array.begin(), array.end(), cmp);
}

} // namespace pystd2026
