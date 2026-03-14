// SPDX-License-Identifier: Apache-2.0
// Copyright 2025-2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<int index, typename... T> constexpr size_t compute_size() {
    if constexpr(index >= sizeof...(T)) {
        return 0;
    } else {
        // Not the best place for this, but here we get it for free.
        static_assert(!is_const_v<T...[index]>);
        return maxval(sizeof(T...[index]), compute_size<index + 1, T...>());
    }
}

template<int index, typename... T> constexpr size_t compute_alignment() {
    if constexpr(index >= sizeof...(T)) {
        return 0;
    } else {
        return maxval(alignof(T...[index]), compute_alignment<index + 1, T...>());
    }
}

template<typename Desired, int8_t index, WellBehaved... T>
constexpr int8_t get_index_for_type() noexcept {
    if constexpr(index >= sizeof...(T)) {
        static_assert(index < sizeof...(T));
    } else {
        if constexpr(is_same_v<Desired, T...[index]>) {
            return index;
        } else {
            return get_index_for_type<Desired, index + 1, T...>();
        }
    }
}

template<typename Desired, WellBehaved... T> constexpr int8_t get_index_for_type() noexcept {
    return get_index_for_type<Desired, 0, T...>();
}

template<WellBehaved... T> class Variant {

    static constexpr int MAX_TYPES = 30;
    static_assert(sizeof...(T) <= MAX_TYPES);
    static_assert(sizeof...(T) > 0);

public:
    Variant() noexcept {
        type_id = 0;
        new(buf) T...[0]{};
    }
    Variant(const Variant<T...> &o) { copy_value_in(o, false); }
    Variant(Variant<T...> &&o) noexcept { move_to_uninitialized_memory(pystd2026::move(o)); }

    int8_t index() const noexcept { return type_id; }

#define PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(i)                                                     \
    {                                                                                              \
        if constexpr(i == new_type) {                                                              \
            new(buf) T... [i] { pystd2026::move(o) };                                              \
        }                                                                                          \
    }                                                                                              \
    break;

    template<typename InType> constexpr Variant(InType &&o) noexcept {
        constexpr int new_type = get_index_for_type<InType, T...>();
        switch(new_type) {
        case 0:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(0);
        case 1:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(1);
        case 2:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(2);
        case 3:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(3);
        case 4:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(4);
        case 5:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(5);
        case 6:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(6);
        case 7:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(7);
        case 8:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(8);
        case 9:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(9);
        case 10:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(10);
        case 11:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(11);
        case 12:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(12);
        case 13:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(13);
        case 14:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(14);
        case 15:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(15);
        case 16:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(16);
        case 17:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(17);
        case 18:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(18);
        case 19:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(19);
        case 20:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(20);
        case 21:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(21);
        case 22:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(22);
        case 23:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(23);
        case 24:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(24);
        case 25:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(25);
        case 26:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(26);
        case 27:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(27);
        case 28:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(28);
        case 29:
            PYSTD2026_VAR_MOVE_CONSTRUCT_SWITCH(29);
        default:
            internal_failure("Bad variant construction.");
        }

        type_id = new_type;
    }

#define PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(i)                                                     \
    {                                                                                              \
        if constexpr(i == new_type) {                                                              \
            new(buf) T...[i](o);                                                                   \
        }                                                                                          \
    }                                                                                              \
    break;

    template<typename InType> Variant(InType &o) {
        constexpr int new_type = get_index_for_type<InType, T...>();
        switch(new_type) {
        case 0:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(0);
        case 1:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(1);
        case 2:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(2);
        case 3:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(3);
        case 4:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(4);
        case 5:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(5);
        case 6:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(6);
        case 7:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(7);
        case 8:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(8);
        case 9:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(9);
        case 10:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(10);
        case 11:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(11);
        case 12:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(12);
        case 13:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(13);
        case 14:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(14);
        case 15:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(15);
        case 16:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(16);
        case 17:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(17);
        case 18:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(18);
        case 19:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(19);
        case 20:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(20);
        case 21:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(21);
        case 22:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(22);
        case 23:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(23);
        case 24:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(24);
        case 25:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(25);
        case 26:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(26);
        case 27:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(27);
        case 28:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(28);
        case 29:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(29);
        default:
            internal_failure("Bad variant construction.");
        }

        type_id = new_type;
    }

    template<typename InType> Variant(const InType &o) {
        constexpr int new_type = get_index_for_type<InType, T...>();
        switch(new_type) {
        case 0:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(0);
        case 1:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(1);
        case 2:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(2);
        case 3:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(3);
        case 4:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(4);
        case 5:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(5);
        case 6:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(6);
        case 7:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(7);
        case 8:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(8);
        case 9:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(9);
        case 10:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(10);
        case 11:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(11);
        case 12:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(12);
        case 13:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(13);
        case 14:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(14);
        case 15:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(15);
        case 16:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(16);
        case 17:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(17);
        case 18:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(18);
        case 19:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(19);
        case 20:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(20);
        case 21:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(21);
        case 22:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(22);
        case 23:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(23);
        case 24:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(24);
        case 25:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(25);
        case 26:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(26);
        case 27:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(27);
        case 28:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(28);
        case 29:
            PYSTD2026_VAR_COPY_CONSTRUCT_SWITCH(29);
        default:
            internal_failure("Bad variant construction.");
        }

        type_id = new_type;
    }

    ~Variant() { destroy(); }

    template<WellBehaved Q> bool contains() const noexcept {
        const auto id = get_index_for_type<Q, T...>();
        return id == type_id;
    }

    template<typename Desired> Desired *get_if() noexcept {
        const int computed_type = get_index_for_type<Desired, T...>();
        if(computed_type == type_id) {
            return reinterpret_cast<Desired *>(buf);
        }
        return nullptr;
    }

    template<typename Desired> const Desired *get_if() const noexcept {
        const int computed_type = get_index_for_type<Desired, T...>();
        if(computed_type == type_id) {
            return reinterpret_cast<const Desired *>(buf);
        }
        return nullptr;
    }

    template<typename Desired> Desired &get() {
        auto *ptr = get_if<Desired>();
        if(!ptr) {
            throw PyException("Variant does not hold the requested type.");
        }
        return *ptr;
    }

    template<typename Desired> const Desired &get() const {
        auto *ptr = get_if<Desired>();
        if(!ptr) {
            throw PyException("Variant does not hold the requested type.");
        }
        return *ptr;
    }

    template<typename OBJ> void insert(OBJ o) noexcept {
        constexpr int new_type = get_index_for_type<OBJ, T...>();
        destroy();
        new(buf) OBJ(move(o));
        type_id = new_type;
    }

    Variant<T...> &operator=(Variant<T...> &&o) noexcept {
        if(this == &o) {
            return *this;
        }
        destroy();
        move_to_uninitialized_memory(move(o));
        return *this;
    }

    Variant<T...> &operator=(const Variant<T...> &o) noexcept {
        if(this == &o) {
            return *this;
        }
        copy_value_in(o, true);
        return *this;
    }

private:
#define PYSTD2026_VAR_MOVE_SWITCH(i)                                                               \
    {                                                                                              \
        if constexpr(i < sizeof...(T)) {                                                           \
            new(buf) T... [i] { move(o.get<T...[i]>()) };                                          \
        }                                                                                          \
    }                                                                                              \
    break;

    void move_to_uninitialized_memory(Variant<T...> &&o) noexcept {
        switch(o.type_id) {
        case 0:
            PYSTD2026_VAR_MOVE_SWITCH(0);
        case 1:
            PYSTD2026_VAR_MOVE_SWITCH(1);
        case 2:
            PYSTD2026_VAR_MOVE_SWITCH(2);
        case 3:
            PYSTD2026_VAR_MOVE_SWITCH(3);
        case 4:
            PYSTD2026_VAR_MOVE_SWITCH(4);
        case 5:
            PYSTD2026_VAR_MOVE_SWITCH(5);
        case 6:
            PYSTD2026_VAR_MOVE_SWITCH(6);
        case 7:
            PYSTD2026_VAR_MOVE_SWITCH(7);
        case 8:
            PYSTD2026_VAR_MOVE_SWITCH(8);
        case 9:
            PYSTD2026_VAR_MOVE_SWITCH(9);
        case 10:
            PYSTD2026_VAR_MOVE_SWITCH(10);
        case 11:
            PYSTD2026_VAR_MOVE_SWITCH(11);
        case 12:
            PYSTD2026_VAR_MOVE_SWITCH(12);
        case 13:
            PYSTD2026_VAR_MOVE_SWITCH(13);
        case 14:
            PYSTD2026_VAR_MOVE_SWITCH(14);
        case 15:
            PYSTD2026_VAR_MOVE_SWITCH(15);
        case 16:
            PYSTD2026_VAR_MOVE_SWITCH(16);
        case 17:
            PYSTD2026_VAR_MOVE_SWITCH(17);
        case 18:
            PYSTD2026_VAR_MOVE_SWITCH(18);
        case 19:
            PYSTD2026_VAR_MOVE_SWITCH(19);
        case 20:
            PYSTD2026_VAR_MOVE_SWITCH(20);
        case 21:
            PYSTD2026_VAR_MOVE_SWITCH(21);
        case 22:
            PYSTD2026_VAR_MOVE_SWITCH(22);
        case 23:
            PYSTD2026_VAR_MOVE_SWITCH(23);
        case 24:
            PYSTD2026_VAR_MOVE_SWITCH(24);
        case 25:
            PYSTD2026_VAR_MOVE_SWITCH(25);
        case 26:
            PYSTD2026_VAR_MOVE_SWITCH(26);
        case 27:
            PYSTD2026_VAR_MOVE_SWITCH(27);
        case 28:
            PYSTD2026_VAR_MOVE_SWITCH(28);
        case 29:
            PYSTD2026_VAR_MOVE_SWITCH(29);
        default:
            internal_failure("Unreachable code in variant move.");
        }
        type_id = o.type_id;
    }

// FIXME, update to do a constexpr check to see if the
// value can be nothrow copied.
#define PYSTD2026_VAR_COPY_SWITCH(i)                                                               \
    {                                                                                              \
        if constexpr(i < sizeof...(T)) {                                                           \
            T...[i] tmp(o.get<T...[i]>());                                                         \
            if(has_existing_value) {                                                               \
                destroy();                                                                         \
            }                                                                                      \
            new(buf) T...[i](move(tmp));                                                           \
        }                                                                                          \
    }                                                                                              \
    break;

    void copy_value_in(const Variant<T...> &o, bool has_existing_value) {
        // If copy construction throws, keep the old value.
        switch(o.type_id) {
        case 0:
            PYSTD2026_VAR_COPY_SWITCH(0);
        case 1:
            PYSTD2026_VAR_COPY_SWITCH(1);
        case 2:
            PYSTD2026_VAR_COPY_SWITCH(2);
        case 3:
            PYSTD2026_VAR_COPY_SWITCH(3);
        case 4:
            PYSTD2026_VAR_COPY_SWITCH(4);
        case 5:
            PYSTD2026_VAR_COPY_SWITCH(5);
        case 6:
            PYSTD2026_VAR_COPY_SWITCH(6);
        case 7:
            PYSTD2026_VAR_COPY_SWITCH(7);
        case 8:
            PYSTD2026_VAR_COPY_SWITCH(8);
        case 9:
            PYSTD2026_VAR_COPY_SWITCH(9);
        case 10:
            PYSTD2026_VAR_COPY_SWITCH(10);
        case 11:
            PYSTD2026_VAR_COPY_SWITCH(11);
        case 12:
            PYSTD2026_VAR_COPY_SWITCH(12);
        case 13:
            PYSTD2026_VAR_COPY_SWITCH(13);
        case 14:
            PYSTD2026_VAR_COPY_SWITCH(14);
        case 15:
            PYSTD2026_VAR_COPY_SWITCH(15);
        case 16:
            PYSTD2026_VAR_COPY_SWITCH(16);
        case 17:
            PYSTD2026_VAR_COPY_SWITCH(17);
        case 18:
            PYSTD2026_VAR_COPY_SWITCH(18);
        case 19:
            PYSTD2026_VAR_COPY_SWITCH(19);
        case 20:
            PYSTD2026_VAR_COPY_SWITCH(20);
        case 21:
            PYSTD2026_VAR_COPY_SWITCH(21);
        case 22:
            PYSTD2026_VAR_COPY_SWITCH(22);
        case 23:
            PYSTD2026_VAR_COPY_SWITCH(23);
        case 24:
            PYSTD2026_VAR_COPY_SWITCH(24);
        case 25:
            PYSTD2026_VAR_COPY_SWITCH(25);
        case 26:
            PYSTD2026_VAR_COPY_SWITCH(26);
        case 27:
            PYSTD2026_VAR_COPY_SWITCH(27);
        case 28:
            PYSTD2026_VAR_COPY_SWITCH(28);
        case 29:
            PYSTD2026_VAR_COPY_SWITCH(29);
        default:
            internal_failure("Unreachable code in variant copy.");
        }

        type_id = o.type_id;
    }

#define PYSTD2026_VAR_COMPARE_SWITCH(i)                                                            \
    {                                                                                              \
        const T...[i] &v1 = get<T...[i]>();                                                        \
        const T...[i] &v2 = o.get<T...[i]>();                                                      \
        return v1 == v2;                                                                           \
    }

    bool operator==(const Variant &o) const {
        if(this == &o) {
            return true;
        }
        if(type_id != o.type_id) {
            return false;
        }
        switch(o.type_id) {
        case 0:
            PYSTD2026_VAR_COMPARE_SWITCH(0);
        case 1:
            PYSTD2026_VAR_COMPARE_SWITCH(1);
        case 2:
            PYSTD2026_VAR_COMPARE_SWITCH(2);
        case 3:
            PYSTD2026_VAR_COMPARE_SWITCH(3);
        case 4:
            PYSTD2026_VAR_COMPARE_SWITCH(4);
        case 5:
            PYSTD2026_VAR_COMPARE_SWITCH(5);
        case 6:
            PYSTD2026_VAR_COMPARE_SWITCH(6);
        case 7:
            PYSTD2026_VAR_COMPARE_SWITCH(7);
        case 8:
            PYSTD2026_VAR_COMPARE_SWITCH(8);
        case 9:
            PYSTD2026_VAR_COMPARE_SWITCH(9);
        case 10:
            PYSTD2026_VAR_COMPARE_SWITCH(10);
        case 11:
            PYSTD2026_VAR_COMPARE_SWITCH(11);
        case 12:
            PYSTD2026_VAR_COMPARE_SWITCH(12);
        case 13:
            PYSTD2026_VAR_COMPARE_SWITCH(13);
        case 14:
            PYSTD2026_VAR_COMPARE_SWITCH(14);
        case 15:
            PYSTD2026_VAR_COMPARE_SWITCH(15);
        case 16:
            PYSTD2026_VAR_COMPARE_SWITCH(16);
        case 17:
            PYSTD2026_VAR_COMPARE_SWITCH(17);
        case 18:
            PYSTD2026_VAR_COMPARE_SWITCH(18);
        case 19:
            PYSTD2026_VAR_COMPARE_SWITCH(19);
        case 20:
            PYSTD2026_VAR_COMPARE_SWITCH(20);
        case 21:
            PYSTD2026_VAR_COMPARE_SWITCH(21);
        case 22:
            PYSTD2026_VAR_COMPARE_SWITCH(22);
        case 23:
            PYSTD2026_VAR_COMPARE_SWITCH(23);
        case 24:
            PYSTD2026_VAR_COMPARE_SWITCH(24);
        case 25:
            PYSTD2026_VAR_COMPARE_SWITCH(25);
        case 26:
            PYSTD2026_VAR_COMPARE_SWITCH(26);
        case 27:
            PYSTD2026_VAR_COMPARE_SWITCH(27);
        case 28:
            PYSTD2026_VAR_COMPARE_SWITCH(28);
        case 29:
            PYSTD2026_VAR_COMPARE_SWITCH(29);
        }
        internal_failure("Unreachable code in variant compare.");
    }

    void destroy() noexcept {
        if(type_id == 0) {
            destroy_by_index<0>();
        } else if(type_id == 1) {
            destroy_by_index<1>();
        } else if(type_id == 2) {
            destroy_by_index<2>();
        } else if(type_id == 3) {
            destroy_by_index<3>();
        } else if(type_id == 4) {
            destroy_by_index<4>();
        } else if(type_id == 5) {
            destroy_by_index<5>();
        } else if(type_id == 6) {
            destroy_by_index<6>();
        } else if(type_id == 7) {
            destroy_by_index<7>();
        } else if(type_id == 8) {
            destroy_by_index<8>();
        } else if(type_id == 9) {
            destroy_by_index<9>();
        } else if(type_id == 10) {
            destroy_by_index<10>();
        } else if(type_id == 11) {
            destroy_by_index<11>();
        } else if(type_id == 12) {
            destroy_by_index<12>();
        } else if(type_id == 13) {
            destroy_by_index<13>();
        } else if(type_id == 14) {
            destroy_by_index<14>();
        } else if(type_id == 15) {
            destroy_by_index<15>();
        } else if(type_id == 16) {
            destroy_by_index<16>();
        } else if(type_id == 17) {
            destroy_by_index<17>();
        } else if(type_id == 18) {
            destroy_by_index<18>();
        } else if(type_id == 19) {
            destroy_by_index<19>();
        } else if(type_id == 20) {
            destroy_by_index<20>();
        } else if(type_id == 21) {
            destroy_by_index<21>();
        } else if(type_id == 22) {
            destroy_by_index<22>();
        } else if(type_id == 23) {
            destroy_by_index<23>();
        } else if(type_id == 24) {
            destroy_by_index<24>();
        } else if(type_id == 25) {
            destroy_by_index<25>();
        } else if(type_id == 26) {
            destroy_by_index<26>();
        } else if(type_id == 27) {
            destroy_by_index<27>();
        } else if(type_id == 28) {
            destroy_by_index<28>();
        } else if(type_id == 29) {
            destroy_by_index<29>();
        } else {
            internal_failure("Unreachable code in variant destroy.");
        }
    }

    template<int index> constexpr void destroy_by_index() noexcept {
        if constexpr(index >= 0 && index < sizeof...(T)) {
            using curtype = T...[index];
            reinterpret_cast<curtype *>(buf)->~curtype();
        }
        type_id = -1;
    }

    alignas(compute_alignment<0, T...>()) char buf[compute_size<0, T...>()];
    int8_t type_id;
};

} // namespace pystd2026
