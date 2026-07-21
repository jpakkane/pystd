// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/common.hpp>

namespace pystd2026 {

// Intended to only be used with small number of items.
// Pretty much all operations are O(n).

template<WellBehaved T, size_t MAX_SIZE> class FixedSet {
public:
    bool try_insert(const T &entry) noexcept {
        auto *eptr = find(entry);
        if(eptr != nullptr) {
            return true;
        }
        return backing.try_push_back(entry);
    }

    void insert(const T &entry) {
        if(!try_insert(entry)) {
            throw ::pystd2026::PyException("Tried to insert to a full FixedSet.");
        }
    }

    bool contains(const T &entry) const noexcept { return find(entry) != nullptr; }

    bool erase(const T &entry) noexcept {
        for(size_t i = 0; i < backing.size(); ++i) {
            if(backing[i] == entry) {
                const size_t last_index = backing.size() - 1;
                if(i != last_index) {
                    ::pystd2026::swap(backing[i], backing[last_index]);
                }
                backing.pop_back();
                return true;
            }
        }
        return false;
    }

    size_t size() const noexcept { return backing.size(); }

    bool is_empty() const noexcept { return backing.is_empty(); }

    bool is_full() const noexcept { return backing.is_full(); }

    T *begin() noexcept { return backing.begin(); }
    T *end() noexcept { return backing.end(); }

    const T *cbegin() const noexcept { return backing.cbegin(); }
    const T *cend() const noexcept { return backing.cend(); }

private:
    const T *find(const T &e) const noexcept {
        for(const auto &element : backing) {
            if(e == element) {
                return &e;
            }
        }
        return nullptr;
    }

    ::pystd2026::FixedVector<T, MAX_SIZE> backing;
};

} // namespace pystd2026
