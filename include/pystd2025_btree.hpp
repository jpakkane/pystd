// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2025.hpp>
#include <assert.h>

namespace pystd2025 {

template<typename T, size_t MAX_SIZE> class FixedVector {
public:
    ~FixedVector() { destroy_contents(); }

    bool try_push_back(const T &obj) noexcept {
        if(num_entries >= MAX_SIZE) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(obj);
        ++num_entries;
        return true;
    }

    bool try_push_back(T &&obj) noexcept {
        if(num_entries >= MAX_SIZE) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(::pystd2025::move(obj));
        ++num_entries;
        return true;
    }

    size_t capacity() const noexcept { return MAX_SIZE; }

    size_t size() const noexcept { return num_entries; }

    bool is_empty() const noexcept { return size() == 0; }

    void pop_back() noexcept {
        if(num_entries == 0) {
            return;
        }
        T *obj = objptr(num_entries - 1);
        obj->~T();
        --num_entries;
    }

    void delete_at(size_t i) {
        if(i >= size()) {
            throw PyException("OOB in delete_at.");
        }
        ++i;
        while(i < size()) {
            *objptr(i - 1) = pystd2025::move(*objptr(i));
        }
        pop_back();
    }

    T *data() noexcept { return (T *)backing; }

    T &operator[](size_t i) {
        if(i >= num_entries) {
            throw "Vector index out of bounds.";
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= num_entries) {
            throw "Vector index out of bounds.";
        }
        return *objptr(i);
    }

private:
    T *objptr(size_t i) { return reinterpret_cast<T *>(rawptr(i)); }

    const T *objptr(size_t i) const { return reinterpret_cast<const T *>(rawptr(i)); }

    char *rawptr(size_t i) noexcept {
        if(i >= MAX_SIZE) {
            throw PyException("OOB in FixedVector.");
        }
        return backing + i * sizeof(T);
    }
    const char *rawptr(size_t i) const noexcept {
        if(i >= MAX_SIZE) {
            throw PyException("OOB in FixedVector.");
        }
        return backing + i * sizeof(T);
    }

    void destroy_contents() noexcept {
        for(size_t i = 0; i < num_entries; ++i) {
            objptr(i)->~T();
        }
        num_entries = 0;
    }

    char backing[MAX_SIZE * sizeof(T)] alignas(alignof(T));
    size_t num_entries = 0;
};

template<WellBehaved Payload, size_t EntryCount> class BTree {
public:
    void insert(const Payload &value) { ++num_entries; }

    size_t size() const { return num_entries; }

private:
    static constexpr size_t NULL_REF = (size_t)-1;

    struct Node {
        // values
        // children
    };

    static_assert(EntryCount % 2 == 1);
    static_assert(EntryCount > 3);
    size_t root = NULL_REF;
    size_t num_entries = 0;
    Vector<Node> nodes;
};

} // namespace pystd2025
