// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <pystd_config.hpp>

#ifndef __GLIBCXX__
void *operator new(size_t, void *ptr) noexcept;
#endif

namespace pystd2026 {

enum class align_val_t : size_t {};

// PyException stores its message as
// UTF-8. Thus no code that is needed
// to implement U8String can throw
// PyExceptions. This function is used
// to break the dependency loop,
// nobody else should use it.
[[noreturn]]
void bootstrap_throw(const char *msg);

template<class T> struct remove_reference {
    typedef T type;
};
template<class T> struct remove_reference<T &> {
    typedef T type;
};
template<class T> struct remove_reference<T &&> {
    typedef T type;
};
template<class T> using remove_reference_t = typename remove_reference<T>::type;

template<class T> constexpr remove_reference_t<T> &&move(T &&t) noexcept {
    return static_cast<typename remove_reference<T>::type &&>(t);
}

struct true_type {
    static constexpr bool value = true;
};

struct false_type {
    static constexpr bool value = false;
};

template<typename U, typename V> struct is_same : false_type {};

template<typename U> struct is_same<U, U> : true_type {};

template<typename U, typename V> constexpr bool is_same_v = is_same<U, V>::value;

template<class T> struct is_const : pystd2026::false_type {};
template<class T> struct is_const<const T> : pystd2026::true_type {};
template<class T> constexpr bool is_const_v = is_const<T>::value;

template<class T> struct is_volatile : pystd2026::false_type {};
template<class T> struct is_volatile<volatile T> : pystd2026::true_type {};
template<class T> constexpr bool is_volatile_v = is_volatile<T>::value;

template<class T> struct is_lvalue_reference : pystd2026::false_type {};
template<class T> struct is_lvalue_reference<T &> : pystd2026::true_type {};

template<typename T>
constexpr T &&forward(typename pystd2026::remove_reference<T>::type &t) noexcept {
    return static_cast<T &&>(t);
}

template<typename T>
constexpr T &&forward(typename pystd2026::remove_reference<T>::type &&t) noexcept {
    static_assert(!pystd2026::is_lvalue_reference<T>::value,
                  "Forward may not be used to convert an rvalue to an lvalue");
    return static_cast<T &&>(t);
}

[[noreturn]] void internal_failure(const char *message) noexcept;

// This is still WIP. But basically require that objects of the
// given type can be default constructed and moved without exceptions.
template<typename T>
concept WellBehaved = requires(T a, T &b, T &&c) {
    requires noexcept(a = ::pystd2026::move(a));
    requires noexcept(a = ::pystd2026::move(b));
    requires noexcept(b = ::pystd2026::move(a));
    requires noexcept(a = ::pystd2026::move(c));
    requires noexcept(T{});
    requires noexcept(T{::pystd2026::move(a)});
    requires noexcept(T{::pystd2026::move(b)});
    requires noexcept(T{::pystd2026::move(c)});
    requires !is_volatile_v<T>;
};

template<typename T>
concept BasicIterator = pystd2026::WellBehaved<T> && requires(T a) {
    ++a;
    a++;
    a + 5;
    a - 5;
    a += 5;
    a -= 5;
    *a;
    // requires pystd2026::WellBehaved<pystd2026::remove_reference_t<decltype(*a)>>;
};

template<typename T1, typename T2> constexpr int maxval(const T1 &a, const T2 &b) {
    return a < b ? b : a;
}

template<typename T1, typename T2> constexpr int minval(const T1 &a, const T2 &b) {
    return a < b ? a : b;
}

struct Monostate {};

class SimpleHash final {
public:
    void feed_bytes(const char *buf, size_t bufsize) noexcept;

    size_t get_hash_value() const { return value; }

private:
    size_t value{0};
};

template<typename Hasher, typename Object> struct HashFeeder {};

template<WellBehaved HashAlgo = SimpleHash> class Hasher {
public:
    template<typename Object> void feed_hash(const Object &o) {
        HashFeeder<Hasher, Object> f;
        f(*this, o);
    }

    void feed_bytes(const char *buf, size_t bufsize) { h.feed_bytes(buf, bufsize); }

    void feed_hash(const int8_t o) { h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o)); }

    void feed_hash(const uint8_t o) { h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o)); }

    void feed_hash(const int16_t o) { h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o)); }

    void feed_hash(const uint16_t o) {
        h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o));
    }

    void feed_hash(const int32_t o) { h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o)); }

    void feed_hash(const uint32_t o) {
        h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o));
    }

    void feed_hash(const int64_t o) { h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o)); }

    void feed_hash(const uint64_t o) {
        h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o));
    }

#if defined(__APPLE__)
    void feed_hash(const unsigned long o) {
        h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(o));
    }
#endif

    template<typename T> void feed_hash(const T *o) {
        h.feed_bytes(reinterpret_cast<const char *>(&o), sizeof(T *));
    }

    size_t get_hash_value() const { return h.get_hash_value(); }

private:
    HashAlgo h;
};

template<WellBehaved E> class Unexpected {
public:
    Unexpected() noexcept : e{} {}
    Unexpected(E &&e_) noexcept : e{pystd2026::move(e_)} {}
    Unexpected(const E &e_) noexcept : e(e_) {}

    E &error() & noexcept { return e; }
    E &&error() && noexcept { return e; }
    const E &error() const & noexcept { return e; }
    const E &&error() const && noexcept { return e; }

    Unexpected &operator=(Unexpected &&o) = default;

private:
    E e;
};

template<WellBehaved V, WellBehaved E> class Expected {
private:
    enum class UnionState : unsigned char {
        Empty,
        Value,
        Error,
    };

    static_assert(!is_same_v<V, E>);

public:
    Expected() noexcept : state{UnionState::Empty} {}

    Expected(Unexpected<E> &&e) noexcept : state{UnionState::Error} {
        new(content) E(pystd2026::move(e.error()));
    }

    Expected(const V &v) noexcept : state{UnionState::Value} { new(content) V(v); }

    Expected(V &&v) noexcept : state{UnionState::Value} { new(content) V(pystd2026::move(v)); }

    Expected(const E &e) noexcept : state{UnionState::Error} { new(content) E(e); }
    Expected(E &&e) noexcept : state{UnionState::Error} { new(content) E(pystd2026::move(e)); }

    Expected(Expected<V, E> &&o) noexcept : state{o.state} {
        if(o.has_value()) {
            new(content) V(pystd2026::move(o.value()));
        } else {
            new(content) E(pystd2026::move(o.error()));
        }
    }

    ~Expected() { destroy(); }

    bool has_value() const noexcept { return state == UnionState::Value; }

    bool has_error() const noexcept { return state == UnionState::Error; }

    operator bool() const noexcept { return has_value(); }

    void operator=(Expected<V, E> &&o) noexcept {
        if(this != &o) {
            destroy();
            if(o.has_value()) {
                new(content) V(pystd2026::move(*reinterpret_cast<V *>(content)));
                state = UnionState::Value;
            } else if(o.has_error()) {
                new(content) E(pystd2026::move(*reinterpret_cast<E *>(o.content)));
                state = UnionState::Error;
            } else {
                state = UnionState::Empty;
            }
            o.destroy();
        }
    }

    V &value() {
        if(*this) {
            abort();
        }
        return *reinterpret_cast<V *>(content);
    }

    E &error() {
        if(!has_error()) {
            abort();
        }
        return *reinterpret_cast<E *>(content);
    }

private:
    void destroy() {
        if(state == UnionState::Value) {
            reinterpret_cast<V *>(content)->~V();
        } else if(state == UnionState::Error) {
            reinterpret_cast<E *>(content)->~E();
        }
        state = UnionState::Empty;
    }
    alignas(maxval(alignof(V), alignof(E))) char content[maxval(sizeof(V), sizeof(E))];
    UnionState state;
};

template<typename T> class EnumerateView {
public:
    explicit EnumerateView(T &o) : underlying{o}, i{0} {};

    template<typename VALTYPE> struct EnumValue {
        size_t i;
        VALTYPE &value;
    };

    template<typename IT> struct EnumIterator {
        size_t i;
        IT it;
        template<typename IT2> bool operator==(const IT2 &o) const { return it == o.it; }
        auto operator*() { return EnumValue{i, *it}; }
        void operator++() {
            ++i;
            ++it;
        }
    };

    auto begin() { return EnumIterator{0, underlying.begin()}; }

    auto end() { return EnumIterator((size_t)-1, underlying.end()); }

private:
    T &underlying;
    size_t i;
};

// Helper class to convert between Pythons .next() based iteration
// and C++'s begin/end based iteration.
template<typename T> class LoopView {
public:
    friend struct T_looper_it;
    struct T_looper_it;
    struct T_looper_sentinel;

    explicit LoopView(T &original) : underlying{original} {}
    LoopView(LoopView &) = delete;
    LoopView(LoopView &&) = delete;

    LoopView() = delete;

    T_looper_it begin() { return T_looper_it{this, underlying.next()}; }

    T_looper_sentinel end() { return T_looper_sentinel{}; }

private:
    T &underlying;

public:
    struct T_looper_sentinel {};
    struct T_looper_it {
        LoopView *orig;
        decltype(orig->underlying.next()) holder;
        bool operator==(const T_looper_sentinel &) const { return !holder; }
        decltype(*holder) &operator*() { return *holder; }
        void operator++() { holder = orig->underlying.next(); }
    };
};

// Same as above, but takes ownership of the passed object.
// The name is a portmanteau of "loop" and "consume".
template<WellBehaved T> class Loopsume {
public:
    friend struct T_looper_it;
    struct T_looper_sentinel;
    struct T_looper_it;

    explicit Loopsume(T original) : underlying{move(original)} {}
    explicit Loopsume(T &original) = delete;
    Loopsume(Loopsume &) = delete;
    Loopsume(Loopsume &&) = delete;

    Loopsume() = delete;

    T_looper_it begin() { return T_looper_it{this, underlying.next()}; }

    T_looper_sentinel end() { return T_looper_sentinel{}; }

private:
    T underlying;

public:
    struct T_looper_it {
        Loopsume *orig;
        decltype(orig->underlying.next()) holder;
        bool operator==(const T_looper_sentinel &) const { return !holder; }
        decltype(*holder) &operator*() { return *holder; }
        void operator++() { holder = orig->underlying.next(); }
    };

    struct T_looper_sentinel {};
};

template<typename T> struct DefaultDeleter {
    static void del(T *t) { delete t; }
};

template<typename T, typename Deleter = DefaultDeleter<T>> class unique_ptr final {
public:
    unique_ptr() noexcept : ptr{nullptr} {}
    explicit unique_ptr(T *t) noexcept : ptr{t} {}
    unique_ptr(const unique_ptr<T, Deleter> &o) = delete;
    unique_ptr(unique_ptr<T, Deleter> &&o) noexcept : ptr{o.ptr} { o.ptr = nullptr; }
    ~unique_ptr() { Deleter::del(ptr); }

    unique_ptr<T, Deleter> &operator=(const unique_ptr<T, Deleter> &o) = delete;

    unique_ptr<T, Deleter> &operator=(unique_ptr<T, Deleter> &&o) noexcept {
        if(this != &o) {
            Deleter::del(ptr);
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }

    void reset(T *new_ptr) {
        if(ptr) {
            Deleter::del(ptr);
        }
        ptr = new_ptr;
    }

    T *release() noexcept {
        T *r = ptr;
        ptr = nullptr;
        return r;
    }

    T *get() noexcept { return ptr; }
    T *get() const noexcept { return ptr; }
    T &operator*() { return *ptr; } // FIXME, check not null maybe?
    T *operator->() noexcept { return ptr; }
    const T *operator->() const noexcept { return ptr; }

    operator bool() const noexcept { return ptr; }

private:
    T *ptr;
};

template<typename T> class unique_arr final {
public:
    unique_arr() noexcept = default;
    explicit unique_arr(size_t size) noexcept : ptr{new T[size]}, array_size{size} {}
    explicit unique_arr(T *t, size_t size = 0) noexcept : ptr{t}, array_size{size} {}
    explicit unique_arr(const unique_arr<T> &o) = delete;
    explicit unique_arr(unique_arr<T> &&o) noexcept : ptr{o.ptr}, array_size{o.array_size} {
        o.ptr = nullptr;
        o.array_size = 0;
    }
    ~unique_arr() { delete[] ptr; }

    unique_arr &operator=(const unique_arr<T> &o) = delete;

    // unique_ptr<T> &
    void operator=(unique_arr<T> &&o) noexcept {
        if(this != &o) {
            delete[] ptr;
            ptr = o.ptr;
            array_size = o.array_size;
            o.ptr = nullptr;
            o.array_size = 0;
        }
        // return *this;
    }

    size_t size() const { return array_size; }
    size_t size_bytes() const { return array_size * sizeof(T); }

    T *get() { return ptr; }
    const T *get() const { return ptr; }
    T *operator->() { return ptr; }

    T &operator[](size_t index) {
        if(index >= array_size) {
            bootstrap_throw("Unique_arr index out of bounds.");
        }
        return ptr[index];
    }

    const T &operator[](size_t index) const {
        if(index >= array_size) {
            bootstrap_throw("Unique_arr index out of bounds.");
        }
        return ptr[index];
    }

private:
    T *ptr = nullptr;
    size_t array_size = 0;
};

template<WellBehaved T> class Optional final {
public:
    Optional() noexcept {
        data.nothing = 0;
        has_value = false;
    }

    Optional(const T &o) {
        new(&data.value) T{o};
        has_value = true;
    }

    Optional(T &&o) noexcept {
        new(&data.value) T{move(o)};
        has_value = true;
    }

    Optional(const Optional<T> &o) {
        if(o) {
            new(&data.value) T(o.data.value);
            has_value = true;
        } else {
            data.nothing = 0;
            has_value = false;
        }
    }

    Optional(Optional<T> &&o) noexcept {
        if(o) {
            new(&data.value) T(move(o.data.value));
            has_value = true;
            o.destroy();
        } else {
            data.nothing = 0;
            has_value = false;
        }
    }

    ~Optional() { destroy(); }

    operator bool() const noexcept { return has_value; }

    void operator=(Optional<T> &&o) noexcept {
        if(this != &o) {
            destroy();
            if(o) {
                new(&data.value) T{pystd2026::move(o.data.value)};
                has_value = true;
                o.destroy();
            }
        }
    }

    void operator=(const Optional<T> &o) noexcept {
        if(this != &o) {
            destroy();
            if(o) {
                new(&data.value) T{o.data.value};
                has_value = true;
            }
        }
    }

    void operator=(T &&o) noexcept {
        if(&o == &data.value) {
            return;
        }
        if(*this) {
            data.value = pystd2026::move(o);
        } else {
            new(&data.value) T{pystd2026::move(o)};
            has_value = true;
        }
    }

    void operator=(const T &o) noexcept {
        if(&o == &data.value) {
            return;
        }
        if(*this) {
            data.value = move(o);
        } else {
            new(&data.value) T{move(o)};
            has_value = true;
        }
    }

    T &operator*() {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return data.value;
    }

    T *operator->() {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return &data.value;
    }

    const T *operator->() const {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return &data.value;
    }

    T &value() {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return data.value;
    }

    const T &value() const {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return data.value;
    }

    const T &operator*() const {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return data.value;
    }

    void reset() { destroy(); }

private:
    void destroy() {
        if(has_value) {
            data.value.~T();
            has_value = false;
            data.nothing = -1;
        }
    }

    union Data {
        T value;
        char nothing;
        Data() {}
        ~Data() {};
    } data;
    bool has_value;
};

enum class EncodingPolicy {
    Enforce,
    Substitute,
    Ignore,
};

// A pointer has an optional null state, so it
// does not need a separate boolean value to specify
// whether it is valid or not.
template<typename T> class Optional<T *> final {
public:
    Optional() noexcept { ptr = nullptr; }

    Optional(T *o) { ptr = o; }

    Optional(T &&o) noexcept { ptr = o.ptr; }

    Optional(const Optional<T *> &o) { ptr = o.ptr; }

    Optional(Optional<T *> &&o) noexcept { ptr = o.ptr; }

    ~Optional() = default;

    operator bool() const noexcept { return ptr; }

    void operator=(Optional<T *> &&o) noexcept { ptr = o.ptr; }

    void operator=(const Optional<T *> &o) noexcept { ptr = o.ptr; }

    void operator=(T &&o) noexcept { ptr = o.ptr; }

    void operator=(const T &o) noexcept { ptr = o.ptr; }

    T &operator*() {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return *ptr;
    }

    T *operator->() {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return ptr;
    }

    const T *operator->() const {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return ptr;
    }

    T *value() {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return ptr;
    }

    const T *value() const {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return ptr;
    }

    const T &operator*() const {
        if(!*this) {
            bootstrap_throw("Empty optional.");
        }
        return *ptr;
    }

    void reset() { ptr = nullptr; }

private:
    T *ptr;
};

static_assert(sizeof(Optional<int *>) == sizeof(int *));

class PyException;

class Bytes;

class BytesView {
public:
    BytesView() noexcept : buf{nullptr}, bufsize{0} {}
    BytesView(const char *buf_, size_t bufsize_) noexcept : buf{buf_}, bufsize{bufsize_} {}
    BytesView(BytesView &&o) noexcept = default;
    BytesView(const BytesView &o) noexcept = default;
    BytesView(const Bytes &b) noexcept;

    BytesView &operator=(BytesView &&o) noexcept = default;
    BytesView &operator=(const BytesView &o) noexcept = default;

    const char *data() const noexcept { return buf; }
    size_t size() const noexcept { return bufsize; }
    size_t size_bytes() const noexcept { return bufsize; }

    bool is_empty() const noexcept { return bufsize == 0; }

    char operator[](size_t i) const {
        if(!buf || i >= bufsize) {
            bootstrap_throw("OOB access in BytesView.");
        }
        return buf[i];
    }

    char at(size_t i) const { return (*this)[i]; }

    BytesView subview(size_t loc, size_t size = -1) const {
        if(size == (size_t)-1) {
            if(loc > bufsize) {
                bootstrap_throw("OOB error in BytesView.");
            }
            return BytesView(buf + loc, bufsize - loc);
        } else {
            if(loc + size > bufsize) {
                bootstrap_throw("OOB error in BytesView.");
            }
            return BytesView(buf + loc, size);
        }
    }

    const char *begin() const { return buf; }

    const char *end() const { return buf + bufsize; }

private:
    const char *buf;
    size_t bufsize;
};

class Bytes {
public:
    Bytes() noexcept;
    explicit Bytes(size_t initial_size) noexcept;
    Bytes(const char *buf, size_t bufsize) noexcept;
    Bytes(const char *buf_start, const char *buf_end);
    Bytes(size_t count, char fill_value);
    Bytes(Bytes &&o) noexcept;
    Bytes(const Bytes &o) noexcept;
    const char *data() const { return buf.get(); }
    char *data() { return buf.get(); }

    void append(const char c);

    void append(const char *begin, const char *end);

    bool is_empty() const { return bufsize == 0; }
    void clear() noexcept { bufsize = 0; }

    size_t size() const { return bufsize; }

    size_t capacity() const { return buf.size_bytes(); }

    void reserve(size_t new_size) { grow_to(new_size); }

    void extend(size_t num_bytes) noexcept;
    void shrink(size_t num_bytes) noexcept;
    void resize_to(size_t num_bytes) noexcept;
    void resize(size_t num_bytes) noexcept { resize_to(num_bytes); }

    void assign(const char *buf_in, size_t in_size);
    void insert(size_t i, const char *buf_in, size_t in_size);

    void push_back(const char c) { append(c); }

    void pop_back(size_t num = 1);

    void pop_front(size_t num = 1);

    char operator[](size_t i) const { return buf[i]; }

    Bytes &operator=(const Bytes &) noexcept;

    Bytes &operator+=(const Bytes &o) noexcept;

    void operator=(Bytes &&o) noexcept {
        if(this != &o) {
            buf = move(o.buf);
            bufsize = o.bufsize;
            o.bufsize = 0;
        }
    }

    bool operator==(const Bytes &o) const noexcept {
        if(bufsize != o.bufsize) {
            return false;
        }
        for(size_t i = 0; i < bufsize; ++i) {
            if(buf[i] != o.buf[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<(const Bytes &o) const {
        const auto num_its = bufsize < o.bufsize ? bufsize : o.bufsize;
        for(size_t i = 0; i < num_its; ++i) {
            const auto &c1 = buf[i];
            const auto &c2 = o.buf[i];
            if(c1 < c2) {
                return true;
            }
            if(c2 > c1) {
                return false;
            }
        }
        return bufsize < o.bufsize;
    }

    template<typename Hasher> void feed_hash(Hasher &h) const { h.feed_bytes(buf.get(), bufsize); }

    bool is_ptr_within(const char *ptr) const {
        return ptr >= buf.get() && ptr < buf.get() + bufsize;
    }

    char front() const;
    char back() const;

    BytesView view() const noexcept { return BytesView(buf.get(), bufsize); }

    const char *begin() const noexcept { return buf.get(); }

    const char *end() const noexcept { return buf.get() + bufsize; }

    void remove(size_t from, size_t to);

private:
    void grow_to(size_t new_size);

    unique_arr<char> buf; // Not zero terminated.
    size_t bufsize;
};

template<WellBehaved T> class Vector final {

public:
    Vector() noexcept = default;

    Vector(const Vector<T> &o) {
        reserve(o.num_entries);
        for(const auto &i : o) {
            push_back(i);
        }
    }

    template<typename Iter1, typename Iter2> Vector(Iter1 start, Iter2 end) {
        reserve(end - start);
        while(start != end) {
            push_back(*start);
            ++start;
        }
    }

    Vector(Vector<T> &&o) noexcept
        : buffer(o.buffer), buf_capacity(o.buf_capacity), num_entries{o.num_entries} {
        o.buffer = nullptr;
        o.buf_capacity = 0;
        o.num_entries = 0;
    }

    ~Vector() {
        deallocate_objects();
        free(buffer);
    }

    void push_back(const T &obj) noexcept {
        if(is_ptr_within(&obj) && needs_to_grow_for(1)) {
            // Fixme, maybe compute index to the backing store
            // and then use that, skipping the temporary.
            T tmp{obj};
            reserve(num_entries + 1);
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(::pystd2026::move(tmp));
            ++num_entries;
        } else {
            reserve(num_entries + 1);
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(obj);
            ++num_entries;
        }
    }

    void push_back(T &&obj) noexcept {
        if(is_ptr_within(&obj) && needs_to_grow_for(1)) {
            // Fixme, maybe compute index to the backing store
            // and then use that, skipping the temporary.
            T tmp{::pystd2026::move(obj)};
            reserve(num_entries + 1);
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(::pystd2026::move(tmp));
            ++num_entries;
        } else {
            reserve(num_entries + 1);
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(::pystd2026::move(obj));
            ++num_entries;
        }
    }

    void emplace_back(auto &&...args) noexcept {
        if constexpr(sizeof...(args) == 1 && pystd2026::is_same_v<decltype(args...[0]), T>) {
            this->push_back(pystd2026::forward(args...[0]));
        } else {
            reserve(num_entries + 1);
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(pystd2026::forward<decltype(args)>(args)...);
            ++num_entries;
        }
    }

    template<typename Iter1, typename Iter2> void append(Iter1 start, Iter2 end) {
        if(is_ptr_within((T *)&(*start))) {
            bootstrap_throw("FIXME, appending contents of vector not handled yet.");
        }
        while(start != end) {
            push_back(*start);
            ++start;
        }
    }

    template<typename Iter1, typename Iter2> void assign(Iter1 start, Iter2 end) {
        if(is_ptr_within(&(*start))) {
            bootstrap_throw("FIXME, assigning subset of vector itself is not supported.");
        }
        clear();
        append(start, end);
    }

    Optional<T> pop_back() noexcept {
        if(num_entries == 0) {
            return {};
        }
        T *obj = objptr(num_entries - 1);
        Optional<T> retval(move(*obj));
        obj->~T();
        // FIXME: shrink if needed.
        --num_entries;
        return retval;
    }

    size_t capacity() const noexcept { return buf_capacity; }
    size_t size() const noexcept { return num_entries; }

    bool is_empty() const noexcept { return size() == 0; }

    void clear() noexcept {
        while(!is_empty()) {
            pop_back();
        }
    }

    T &front() {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[0];
    }

    const T &front() const {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[0];
    }

    T &back() {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    const T &back() const {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    T *data() { return reinterpret_cast<T *>(buffer); }

    T &operator[](size_t i) {
        if(i >= num_entries) {
            bootstrap_throw("Vector index out of bounds.");
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= num_entries) {
            bootstrap_throw("Vector index out of bounds.");
        }
        return *objptr(i);
    }

    T &at(size_t i) { return (*this)[i]; }

    const T &at(size_t i) const { return (*this)[i]; }

    Vector<T> &operator=(Vector<T> &&o) noexcept {
        if(this != &o) {
            deallocate_objects();
            free(buffer);
            buffer = o.buffer;
            num_entries = o.num_entries;
            buf_capacity = o.buf_capacity;
            o.buffer = nullptr;
            o.num_entries = 0;
            o.buf_capacity = 0;
        }
        return *this;
    }

    bool operator==(const Vector<T> &o) const noexcept {
        if(this == &o) {
            return true;
        }
        if(o.size() != size()) {
            return false;
        }
        for(size_t i = 0; i < size(); ++i) {
            if((*this)[i] != o[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Vector<T> &o) const noexcept { return !(*this == o); }

    const T *cbegin() const { return objptr(0); }
    const T *cend() const { return objptr(num_entries); }

    T *begin() const { return const_cast<T *>(objptr(0)); }
    T *end() const { return const_cast<T *>(objptr(num_entries)); }

    void reserve(size_t new_size) {
        if(new_size > size()) {
            resize_to(new_size);
        }
    }

private:
    T *objptr(size_t i) noexcept { return reinterpret_cast<T *>(rawptr(i)); }
    const T *objptr(size_t i) const noexcept { return reinterpret_cast<const T *>(rawptr(i)); }
    char *rawptr(size_t i) noexcept { return buffer + i * sizeof(T); }
    const char *rawptr(size_t i) const noexcept { return buffer + i * sizeof(T); }

    void deallocate_objects() {
        for(size_t i = 0; i < num_entries; ++i) {
            objptr(i)->~T();
        }
        num_entries = 0;
    }

    bool needs_to_grow_for(size_t num_new_items) {
        return num_entries + num_new_items > buf_capacity;
    }

    bool is_ptr_within(const T *ptr) const { return ptr >= begin() && ptr < end(); }

    void resize_to(size_t new_capacity) {
        const size_t MIN_CAPACITY = 16;
        if(new_capacity < num_entries) {
            bootstrap_throw("Shrinking too small.");
        }
        if(new_capacity < MIN_CAPACITY) {
            // To avoid multiple small allocations at the beginning.
            new_capacity = MIN_CAPACITY;
        }
        const size_t buffer_size = new_capacity * sizeof(T);
        const size_t end_padding_size = (alignof(T) - (buffer_size % alignof(T))) % alignof(T);
        const size_t allocation_size = buffer_size + end_padding_size;
#if defined __APPLE__
        // If alignment is less than 8, macOS will return a null pointer
        // I have no idea why.
        const size_t alignment = alignof(T) < 8 ? 8 : alignof(T);
#else
        const size_t alignment = alignof(T);
#endif
        char *new_buf = (char *)aligned_alloc(alignment, allocation_size);
        for(size_t i = 0; i < num_entries; ++i) {
            T *obj = objptr(i);
            new(new_buf + i * sizeof(T)) T(::pystd2026::move(*obj));
            obj->~T();
        }
        buf_capacity = new_capacity;
        free(buffer);
        buffer = new_buf;
    }

    char *buffer = nullptr;
    size_t buf_capacity = 0;
    size_t num_entries = 0;
};

template<WellBehaved T, size_t MAX_SIZE> class FixedVector {
public:
    FixedVector() noexcept = default;
    FixedVector(FixedVector &&o) noexcept { swipe_from(o); }

    ~FixedVector() { destroy_contents(); }

    FixedVector &operator=(FixedVector &&o) noexcept {
        if(this != &o) {
            destroy_contents();
            swipe_from(o);
        }
        return *this;
    }

    bool try_push_back(const T &obj) noexcept {
        if(is_full()) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(obj);
        ++num_entries;
        return true;
    }

    bool try_push_back(T &&obj) noexcept {
        if(is_full()) {
            return false;
        }
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(::pystd2026::move(obj));
        ++num_entries;
        return true;
    }

    void push_back(const T &obj) {
        if(!try_push_back(obj)) {
            bootstrap_throw("Tried to push back to a full FixedVector.");
        }
    }

    void push_back(T &&obj) {
        if(!try_push_back(::pystd2026::move(obj))) {
            bootstrap_throw("Tried to push back to a full FixedVector.");
        }
    }

    void insert(size_t loc, T &&obj) {
        if(is_full()) {
            bootstrap_throw("Insert to a full vector.");
        }
        if(loc > size()) {
            bootstrap_throw("Insertion past the end of Fixed vector.");
        }
        if(loc == size()) {
            ++num_entries;
            new(objptr(loc)) T(::pystd2026::move(obj));
            return;
        }
        auto *last = objptr(size() - 1);
        ++num_entries;
        new(objptr(size() - 1)) T(::pystd2026::move(*last));
        for(size_t i = num_entries - 2; i > loc; --i) {
            auto *dst = objptr(i);
            auto *src = objptr(i - 1);
            *dst = ::pystd2026::move(*src);
        }
        *objptr(loc) = ::pystd2026::move(obj);
    }

    size_t capacity() const noexcept { return MAX_SIZE; }

    size_t size() const noexcept { return num_entries; }

    bool is_empty() const noexcept { return size() == 0; }

    bool is_full() const noexcept { return size() >= MAX_SIZE; }

    void pop_back() noexcept {
        if(is_empty()) {
            return;
        }
        T *obj = objptr(num_entries - 1);
        obj->~T();
        --num_entries;
    }

    void pop_front() noexcept {
        if(is_empty()) {
            return;
        }
        delete_at(0);
    }

    void delete_at(size_t i) {
        if(i >= size()) {
            bootstrap_throw("OOB in delete_at.");
        }
        ++i;
        while(i < size()) {
            *objptr(i - 1) = ::pystd2026::move(*objptr(i));
            ++i;
        }
        pop_back();
    }

    void move_append(FixedVector &o) {
        if(this == &o) {
            bootstrap_throw("Can not move append self.");
        }
        if(size() + o.size() > MAX_SIZE) {
            bootstrap_throw("Appending would exceed max size.");
        }
        for(auto &i : o) {
            push_back(::pystd2026::move(i));
        }
    }

    T &front() {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[0];
    }

    const T &front() const {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[0];
    }

    T &back() {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    const T &back() const {
        if(is_empty()) {
            bootstrap_throw("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    T *data() noexcept { return reinterpret_cast<T *>(backing); }

    T &operator[](size_t i) {
        if(i >= num_entries) {
            bootstrap_throw("FixedVector index out of bounds.");
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= num_entries) {
            bootstrap_throw("Fixed Vector index out of bounds.");
        }
        return *objptr(i);
    }

    T *begin() const { return const_cast<T *>(objptr(0)); }
    T *end() const { return const_cast<T *>(objptr(num_entries)); }

private:
    T *objptr(size_t i) noexcept { return reinterpret_cast<T *>(rawptr(i)); }
    const T *objptr(size_t i) const noexcept { return reinterpret_cast<const T *>(rawptr(i)); }
    char *rawptr(size_t i) noexcept { return backing + i * sizeof(T); }
    const char *rawptr(size_t i) const noexcept { return backing + i * sizeof(T); }

    void destroy_contents() noexcept {
        for(size_t i = 0; i < num_entries; ++i) {
            objptr(i)->~T();
        }
        num_entries = 0;
    }

    void swipe_from(FixedVector &o) noexcept {
        if(this == &o) {
            internal_failure("Identity confusion.");
        }
        for(size_t i = 0; i < o.num_entries; ++i) {
            auto obj_loc = objptr(i);
            new(obj_loc) T(::pystd2026::move(*o.objptr(i)));
        }
        num_entries = o.num_entries;
        o.destroy_contents();
    }

    alignas(alignof(T)) char backing[MAX_SIZE * sizeof(T)];
    size_t num_entries = 0;
};

// Iterates over the code points of a valid UTF 8 string.
// If the string used is not valid UTF-8, result is undefined.
class ValidatedU8Iterator {
public:
    friend class U8String;
    friend class U8Match;
    friend class U8StringView;

    ValidatedU8Iterator() noexcept = default;

    ValidatedU8Iterator(const ValidatedU8Iterator &) = default;
    ValidatedU8Iterator(ValidatedU8Iterator &&) = default;

    uint32_t operator*();

    ValidatedU8Iterator &operator++();
    ValidatedU8Iterator operator++(int);

    ValidatedU8Iterator &operator--();

    bool operator==(const ValidatedU8Iterator &o) const;
    bool operator!=(const ValidatedU8Iterator &o) const;

    struct CharInfo {
        uint32_t codepoint;
        uint32_t byte_count;
    };

private:
    explicit ValidatedU8Iterator(const unsigned char *utf8_string)
        : buf{utf8_string}, has_char_info{false} {}

    void compute_char_info();

    const unsigned char *byte_location() const { return buf; }

    const unsigned char *buf = nullptr;
    CharInfo char_info;
    bool has_char_info = false;
};

class ValidatedU8ReverseIterator {
public:
    friend class U8String;

    ValidatedU8ReverseIterator(const ValidatedU8ReverseIterator &) = default;
    ValidatedU8ReverseIterator(ValidatedU8ReverseIterator &&) = default;

    uint32_t operator*();

    ValidatedU8ReverseIterator &operator++();
    ValidatedU8ReverseIterator operator++(int);

    bool operator==(const ValidatedU8ReverseIterator &o) const;
    bool operator!=(const ValidatedU8ReverseIterator &o) const;

private:
    explicit ValidatedU8ReverseIterator(const unsigned char *utf8_string, int64_t offset)
        : original_str{utf8_string}, offset{offset}, has_char_info{false} {
        go_backwards();
    }

    void go_backwards();
    void compute_char_info();

    const unsigned char *original_str;
    int64_t offset; // Need to represent -1;
    ValidatedU8Iterator::CharInfo char_info;
    bool has_char_info;
};

class CString;
// No embedded null chars
// NOT guaranteed to be null terminated.
class CStringView {
public:
    CStringView() noexcept : buf{nullptr}, bufsize{0} {}
    CStringView(const char *str) noexcept;
    CStringView(const char *start, const char *stop);
    CStringView(const char *str, size_t length);
    explicit CStringView(const CString &str) noexcept;

    bool operator==(const char *str) const;
    bool operator==(CStringView o) const;
    bool is_empty() const { return bufsize == 0; }

    char front() const;
    bool starts_with(const char *str) const;
    bool starts_with(CStringView view) const;

    const char *data() const { return buf; }
    size_t size() const { return bufsize; }
    size_t find(char c) const;
    CStringView substr(size_t pos = 0, size_t count = (size_t)-1) const;

    char operator[](size_t index) const {
        if(!buf || index > bufsize) {
            bootstrap_throw("OOB in CStringView.");
        }
        return buf[index];
    }

    size_t find(CStringView substr) const;

    char at(size_t index) const { return (*this)[index]; }

    const char *begin() const { return buf; }

    const char *end() const { return buf + bufsize; }

    bool operator<(const CStringView &o) const;

    bool overlaps(CStringView &other) const;

    CString upper() const noexcept;
    CString lower() const noexcept;

private:
    const char *buf;
    size_t bufsize;
};

typedef bool (*CStringViewCallback)(const CStringView &piece, void *ctx);

// A string guaranteed to end with a zero terminator.
class CString {
public:
    CString() noexcept : bytes() { bytes.append('\0'); }
    CString(CString &&o) noexcept = default;
    CString(const CString &o) noexcept = default;
    explicit CString(Bytes incoming);
    CString(const CStringView &view);
    explicit CString(const char *txt, size_t txtsize = -1);

    const char *c_str() const { return bytes.data(); }

    const char *data() const { return bytes.data(); }

    char *mutable_data() { return bytes.data(); }

    CStringView view() const;

    void strip();
    // CString stripped() const;

    size_t size() const;
    size_t length() const { return size(); }

    void clear() noexcept { bytes.clear(); }

    CString substr(size_t offset, size_t length = -1) const;

    char operator[](size_t i) const { return bytes[i]; }

    void operator=(const pystd2026::CString &) noexcept;
    void operator=(CString &&o) noexcept {
        if(this != &o) {
            bytes = move(o.bytes);
        }
    }

    bool operator==(const CString &o) const { return bytes == o.bytes; }
    bool operator<(const CString &o) const { return bytes < o.bytes; }
    // Fixme: add <=>

    bool operator==(const char *str) const;
    bool operator==(const CStringView &o) const;

    CString &operator+=(const CString &o);

    CString &operator+=(char c);

    CString &operator+=(const char *str);

    CString &operator=(const char *);

    void append(const char c);
    void append(const char *str, size_t strsize = -1);
    void append(const char *start, const char *stop);

    template<typename Hasher> void feed_hash(Hasher &h) const { bytes.feed_hash(h); }

    template<typename T = CString> Vector<T> split() const;

    template<typename T = CString> Vector<T> split_by(char c) const;

    void split(CStringViewCallback cb, void *ctx) const;

    void split_by(char c, CStringViewCallback cb, void *ctx) const;

    bool is_empty() const {
        // The buffer always has a null terminator.
        return bytes.size() == 1;
    }

    char front() const { return bytes.front(); }

    char back() const;

    void pop_back() noexcept;
    void push_back(const char c) noexcept { append(c); }

    void insert(size_t i, const CStringView &v) noexcept;

    size_t find(CStringView substr) const;

    size_t rfind(const char c) const noexcept;

    void reserve(size_t new_size) noexcept { bytes.reserve(new_size); }

    void resize(size_t new_size) noexcept { bytes.resize(new_size); }

    void remove(size_t from, size_t to) { bytes.remove(from, to); }

    CString upper() const noexcept { return view().upper(); }
    CString lower() const noexcept { return view().lower(); }

private:
    bool view_points_to_this(const CStringView &v) const;

    void check_embedded_nuls() const;

    Bytes bytes;
};

template<size_t BUF_SIZE> class FixedCString {
    static_assert(BUF_SIZE > 0);

public:
    FixedCString() noexcept {
        buf[0] = '\0';
        strsize = 0;
    }

    FixedCString(FixedCString &&o) noexcept { swipe(o); }

    FixedCString(const FixedCString &o) noexcept {
        for(size_t i = 0; i < o.strsize + 1; ++i) {
            buf[i] = o.buf[i];
        }
        strsize = o.strsize;
    }

    explicit FixedCString(const char *str, const uint32_t length_in = (uint32_t)-1) {
        if(length_in == (uint32_t)-1) {
            for(size_t i = 0; i < BUF_SIZE + 1; ++i) {
                buf[i] = str[i];
                if(buf[i] == '\0') {
                    strsize = i;
                    return;
                }
            }
            bootstrap_throw("Input string too long for FixedCString.");
        }
        if(length_in > BUF_SIZE) {
            bootstrap_throw("Input string too long for FixedCString.");
        }
        for(size_t i = 0; i < length_in + 1; ++i) {
            buf[i] = str[i];
        }
        strsize = length_in;
    }

    FixedCString &operator=(FixedCString &&o) noexcept {
        if(this != &o) {
            swipe(o);
        }
        return *this;
    }

    FixedCString &operator=(const FixedCString &o) noexcept {
        if(this != &o) {
            for(uint32_t i = 0; i < o.strsize + 1; ++i) {
                buf[i] = o.buf[i];
            }
            strsize = o.strsize;
        }
        return *this;
    }

    template<size_t OTHER_BUFSIZE>
    FixedCString &operator=(const FixedCString<OTHER_BUFSIZE> &o) noexcept {
        *this = o.view();
        return *this;
    }

    FixedCString &operator=(const CStringView &o) {
        if(o.data() == buf) {
            return *this;
        }
        if(o.size() > BUF_SIZE) {
            bootstrap_throw("Input string too long for FixedCString.");
        }
        if(o.data() > buf && o.data() < (buf + BUF_SIZE)) {
            bootstrap_throw("Self sub-assignment not yet supported.");
        }

        for(uint32_t i = 0; i < o.size(); ++i) {
            buf[i] = o[i];
        }
        strsize = o.size();
        buf[strsize] = '\0';
        return *this;
    }

    bool operator==(const CStringView &other) noexcept {
        if(strsize != other.size()) {
            return false;
        }
        return view() == other;
    }

    bool operator==(const char *str) noexcept {
        for(size_t i = 0; i < strsize + 1; ++i) {
            if(buf[i] != str[i]) {
                return false;
            }
        }
        return true;
    }

    char &operator[](uint32_t index) {
        if(index > BUF_SIZE) {
            bootstrap_throw("OoB in FixedCString");
        }
        return buf[index];
    }

    const char &operator[](uint32_t index) const {
        if(index > BUF_SIZE) {
            bootstrap_throw("OoB in FixedCString");
        }
        return buf[index];
    }

    CStringView view() const noexcept { return CStringView(buf, strsize); }

    char *c_str() noexcept { return buf; }

    const char *c_str() const noexcept { return buf; }

    bool is_empty() const noexcept { return strsize == 0; }

    size_t size() const noexcept { return strsize; }

    void pop_back() noexcept {
        if(strsize > 0) {
            --strsize;
            buf[strsize] = '\0';
        }
    }

private:
    void swipe(FixedCString &o) noexcept {
        for(size_t i = 0; i < BUF_SIZE + 1; ++i) {
            buf[i] = o.buf[i];
        }
        strsize = o.strsize;
        o.strsize = 0;
        o.buf[0] = '\0';
    }

    char buf[BUF_SIZE + 1];
    uint32_t strsize; // This could even be a uint16_t to save space.
};

class U8String;

class U8StringView {
public:
    U8StringView() {}

    U8StringView(const ValidatedU8Iterator &start_, const ValidatedU8Iterator &end_)
        : start{start_}, stop{end_} {
        if(((bool)start.buf) ^ ((bool)stop.buf)) {
            bootstrap_throw("Mixed nullptr range to U8StringView.");
        }
        if(start.buf > stop.buf) {
            bootstrap_throw("Invalid range to U8StringView.");
        }
    }
    CStringView raw_view() const;

    bool overlaps(const U8StringView &o) const;
    bool operator==(const char *) const;

    U8String upper() const;
    U8String lower() const;

    size_t size_bytes() const noexcept { return stop.buf - start.buf; }

    ValidatedU8Iterator begin() const { return start; };
    ValidatedU8Iterator end() const { return stop; };

    ValidatedU8Iterator cbegin() const { return start; };
    ValidatedU8Iterator cend() const { return stop; };

private:
    ValidatedU8Iterator start;
    ValidatedU8Iterator stop;
};

typedef bool (*U8StringViewCallback)(const U8StringView &piece, void *ctx);
typedef bool (*IsSplittingCharacter)(uint32_t codepoint);

class U8String {
public:
    U8String() noexcept = default;
    U8String(U8String &&o) noexcept = default;
    U8String(const U8String &o) noexcept = default;
    U8String(Bytes incoming);
    explicit U8String(const U8StringView &view);
    explicit U8String(const char *txt, size_t txtsize = -1);
    const char *c_str() const { return cstring.data(); }

    size_t size_bytes() const { return cstring.size(); }
    bool is_empty() const { return cstring.is_empty(); }

    U8String substr(size_t offset, size_t length) const;

    U8StringView view() const;

    template<typename Seq> U8String join(const Seq &sequence) const {
        size_t i = 0;
        U8String result;
        // Fixme, reserve size in advance.
        for(const auto &e : sequence) {
            if(i != 0) {
                result += *this;
            }
            result += e;
            ++i;
        }
        return result;
    }

    U8String &operator=(const U8String &o) {
        U8String tmp(o);
        (*this) = pystd2026::move(tmp);
        return *this;
    }

    void operator=(U8String &&o) noexcept {
        if(this != &o) {
            cstring = move(o.cstring);
        }
    }

    bool operator==(const U8String &o) const { return cstring == o.cstring; }
    bool operator<(const U8String &o) const { return cstring < o.cstring; }
    // Fixme: add <=>

    bool operator==(const char *str) const;

    U8String &operator+=(const U8String &o);

    template<typename Hasher> void feed_hash(Hasher &h) const { cstring.feed_hash(h); }

    // This should not be a template, but due to the concept
    // requirement it needs to be.
    //
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119848
    template<typename T = U8String> Vector<T> split_ascii() const;

    void split(U8StringViewCallback cb, IsSplittingCharacter is_split_char, void *ctx) const;

    ValidatedU8Iterator cbegin() const {
        return ValidatedU8Iterator((const unsigned char *)cstring.data());
    }

    ValidatedU8Iterator cend() const {
        return ValidatedU8Iterator((const unsigned char *)cstring.data() + size_bytes());
    }

    ValidatedU8ReverseIterator crbegin() const {
        return ValidatedU8ReverseIterator((const unsigned char *)cstring.data(), size_bytes());
    }

    ValidatedU8ReverseIterator crend() const {
        return ValidatedU8ReverseIterator((const unsigned char *)cstring.data(), -1);
    }

    void insert(const ValidatedU8Iterator &it, U8StringView view);

    void remove(U8StringView view);
    void pop_front() noexcept;
    void pop_back() noexcept;
    void append_codepoint(uint32_t codepoint);

    bool contains(const U8StringView &view) const;
    bool overlaps(const U8StringView &view) const;

    pystd2026::U8String upper() const { return view().upper(); }

    pystd2026::U8String lower() const { return view().lower(); }

    void reserve(size_t size_in_bytes) noexcept { cstring.reserve(size_in_bytes); }

private:
    CString cstring;
    // Store length in codepoints.
};

class PyException {
public:
    explicit PyException(const char *msg);
    explicit PyException(U8String msg) : message{msg} {}

    const U8String &what() const { return message; }

private:
    U8String message;
};

template<typename T> class Span {
public:
    Span() noexcept : array(nullptr), arraysize(0) {};
    Span(T *src, size_t src_size) noexcept : array(src), arraysize(src_size) {}
    Span(Span &&o) noexcept = default;
    Span(const Span &o) noexcept = default;

    Span &operator=(Span &&o) noexcept = default;
    Span &operator=(const Span &o) noexcept = default;

    const T &operator[](size_t i) const {
        if(i >= arraysize) {
            throw PyException("OOB in span.");
        }
        return array[i];
    }

    T &operator[](size_t i) {
        if(i >= arraysize) {
            throw PyException("OOB in span.");
        }
        return array[i];
    }

    const T &at(size_t i) const { return (*this)[i]; }

    const T *cbegin() const { return array; }
    T *begin() const { return array; }

    const T *cend() const { return array + arraysize; }
    T *end() const { return array + arraysize; }

    size_t size() const noexcept { return arraysize; }

    bool is_empty() const noexcept { return arraysize == 0; }

    Span subspan(size_t offset, size_t subspan_size = (size_t)-1) const {
        if(offset >= arraysize) {
            throw PyException("Offset OoB in subspan.");
        }
        if(subspan_size == (size_t)-1) {
            return Span(array + offset, arraysize - offset);
        }
        if(offset + subspan_size >= arraysize) {
            throw PyException("Subspan goes OoB.");
        }
        return Span(array + offset, subspan_size);
    }

private:
    T *array;
    size_t arraysize;
};

template<typename Hasher> struct HashFeeder<Hasher, U8String> {
    void operator()(Hasher &h, const U8String &u8) noexcept { u8.feed_hash(h); }
};

class File;

class FileEndSentinel final {
public:
private:
};

class FileLineIterator final {
public:
    explicit FileLineIterator(File *f) : f{f} { is_up_to_date = false; }

    bool operator!=(const FileEndSentinel &) const;

    Bytes &&operator*();

    FileLineIterator &operator++() {
        is_up_to_date = false;
        return *this;
    }

private:
    File *f;
    Bytes line;
    bool is_up_to_date = false;
};

class File {
public:
    File(const char *fname, const char *modes);

    Bytes readline_bytes();
    ~File();

    FileEndSentinel end() { return FileEndSentinel(); }
    FileLineIterator begin() { return FileLineIterator(this); }

    File &operator=(File &&o) = delete;
    File &operator=(const File &o) = delete;
    // Vector<U8String> readlines();
    // Vector<Bytes> readlines_raw();

    bool eof() const { return feof(f); }

private:
    FILE *f;
    EncodingPolicy policy;
};

template<typename Hasher> void feed_hash(Hasher &h, int32_t i) noexcept { h.feed_primitive(i); }

template<typename Hasher> void feed_hash(Hasher &h, size_t i) noexcept { h.feed_primitive(i); }

template<typename Key, typename Value> class HashMapIterator;

template<WellBehaved Key, WellBehaved Value, WellBehaved HashAlgo = SimpleHash>
class HashMap final {
public:
    friend class HashMapIterator<Key, Value>;
    HashMap() noexcept {
        salt = (size_t)this;
        num_entries = 0;
        size_in_powers_of_two = 4;
        auto initial_table_size = 1 << size_in_powers_of_two;
        mod_mask = initial_table_size - 1;
        data.md = unique_arr<SlotMetadata>(initial_table_size);
        data.reset_hash_values();
        data.keydata = Bytes(initial_table_size * sizeof(Key));
        data.valuedata = Bytes(initial_table_size * sizeof(Value));
    }

    Value *lookup(const Key &key) const {
        const auto hashval = hash_for(key);
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.md[slot].state == SlotState::Empty) {
                return nullptr;
            } else if(data.md[slot].state == SlotState::Tombstone) {

            } else if(data.md[slot].bloom_matches(hashval)) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    return const_cast<Value *>(data.valueptr(slot));
                }
            }
            slot = (slot + 1) & mod_mask;
        }
    }

    Value &at(const Key &key) {
        auto *v = lookup(key);
        if(!v) {
            throw PyException("Map did not contain requested element.");
        }
        return *v;
    }

    Value &insert(const Key &key, Value v) {
        if(fill_ratio() >= MAX_LOAD) {
            grow();
        }

        const auto hashval = hash_for(key);
        return insert_internal(hashval, key, pystd2026::move(v));
    }

    void remove(const Key &key) {
        const auto hashval = hash_for(key);
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.md[slot].state == SlotState::Empty) {
                return;
            } else if(data.md[slot].state == SlotState::Tombstone) {

            } else if(data.md[slot].bloom_matches(hashval)) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    auto *value_loc = data.valueptr(slot);
                    value_loc->~Value();
                    const auto previous_slot = (slot + table_size() - 1) & mod_mask;
                    const auto next_slot = (slot + 1) & mod_mask;
                    if(data.md[previous_slot].state == SlotState::Empty &&
                       data.md[next_slot].state == SlotState::Empty) {
                        data.md[slot].state = SlotState::Empty;
                    } else {
                        data.md[slot].state = SlotState::Tombstone;
                    }
                    --num_entries;
                    return;
                }
            }
            slot = (slot + 1) & mod_mask;
        }
    }

    Value &operator[](const Key &k) {
        auto *c = lookup(k);
        if(c) {
            return (*c);
        } else {
            return insert(k, Value{});
        }
    }

    bool contains(const Key &key) const { return lookup(key) != nullptr; }

    size_t size() const { return num_entries; }

    bool is_empty() const { return size() == 0; }

    HashMapIterator<Key, Value> begin() const {
        return HashMapIterator<Key, Value>(const_cast<HashMap *>(this), 0);
    }

    HashMapIterator<Key, Value> end() const {
        return HashMapIterator<Key, Value>(const_cast<HashMap *>(this), table_size());
    }

    void clear() {
        data.clear();
        num_entries = 0;
    }

private:
    static constexpr uint8_t BLOOM_MASK = 0b111111;

    enum class SlotState : uint8_t {
        Empty,
        HasValue,
        Tombstone,
    };
    struct SlotMetadata {
        SlotState state : 2;
        uint8_t bloom : 6;

        bool bloom_matches(size_t hashcode) const {
            return bloom == (uint8_t)(hashcode & BLOOM_MASK);
        }
    };

    // This is neither fast, memory efficient nor elegant.
    // At this point in the project life cycle it does not need to be.
    struct MapData {
        unique_arr<SlotMetadata> md;
        Bytes keydata;
        Bytes valuedata;

        MapData() = default;
        MapData(MapData &&o) noexcept {
            md = move(o.md);
            keydata = move(o.keydata);
            valuedata = move(o.valuedata);
        }

        void operator=(MapData &&o) noexcept {
            if(this != &o) {
                md = move(o.md);
                keydata = move(o.keydata);
                valuedata = move(o.valuedata);
            }
        }

        ~MapData() { deallocate_contents(); }

        Key *keyptr(size_t i) noexcept { return (Key *)(keydata.data() + i * sizeof(Key)); }
        const Key *keyptr(size_t i) const noexcept {
            return (const Key *)(keydata.data() + i * sizeof(Key));
        }

        Value *valueptr(size_t i) noexcept {
            return (Value *)(valuedata.data() + i * sizeof(Value));
        }
        const Value *valueptr(size_t i) const noexcept {
            return (const Value *)(valuedata.data() + i * sizeof(Value));
        }

        void reset_hash_values() noexcept { memset(md.get(), 0, md.size_bytes()); }

        void deallocate_contents() noexcept {
            for(size_t i = 0; i < md.size(); ++i) {
                if(md[i].state == SlotState::HasValue) {
                    keyptr(i)->~Key();
                    valueptr(i)->~Value();
                }
            }
        }

        void clear() noexcept {
            deallocate_contents();
            reset_hash_values();
        }

        bool has_value_at(size_t offset) const {
            if(offset >= md.size()) {
                return false;
            }
            return md[offset].state == SlotState::HasValue;
        }
    };

    size_t hash_to_slot(size_t hashval) const {
        const size_t total_bits = sizeof(size_t) * 8;
        size_t consumed_bits = 0;
        size_t slot = 0;
        while(consumed_bits < total_bits) {
            slot ^= hashval & mod_mask;
            consumed_bits += size_in_powers_of_two;
            hashval >>= size_in_powers_of_two;
        }
        return slot;
    }

    Value &insert_internal(size_t hashval, const Key &key, Value &&v) {
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.md[slot].state != SlotState::HasValue) {
                auto *key_loc = data.keyptr(slot);
                auto *value_loc = data.valueptr(slot);
                new(key_loc) Key(key);
                new(value_loc) Value{pystd2026::move(v)};
                data.md[slot] = SlotMetadata{SlotState::HasValue, (uint8_t)(hashval & BLOOM_MASK)};
                ++num_entries;
                return *value_loc;
            }
            if(data.md[slot].bloom_matches(hashval)) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    auto *value_loc = data.valueptr(slot);
                    *value_loc = pystd2026::move(v);
                    return *value_loc;
                }
            }
            slot = (slot + 1) & mod_mask;
        }
    }

    void grow() {
        const auto new_size = 2 * table_size();
        const auto new_powers_of_two = size_in_powers_of_two + 1;
        const auto new_mod_mask = new_size - 1;
        MapData grown;

        grown.md = unique_arr<SlotMetadata>(new_size);
        grown.keydata = Bytes(new_size * sizeof(Key));
        grown.valuedata = Bytes(new_size * sizeof(Value));

        grown.reset_hash_values();
        MapData old = move(data);
        data = move(grown);
        size_in_powers_of_two = new_powers_of_two;
        mod_mask = new_mod_mask;
        num_entries = 0;
        for(size_t i = 0; i < old.md.size(); ++i) {
            if(old.md[i].state == SlotState::HasValue) {
                auto &old_key = *old.keyptr(i);
                auto hashval = hash_for(old_key);
                insert_internal(hashval, old_key, pystd2026::move(*old.valueptr(i)));
            }
        }
    }

    size_t hash_for(const Key &k) const {
        Hasher<HashAlgo> h;
        h.feed_hash(salt);
        h.feed_hash(k);
        auto raw_hash = h.get_hash_value();
        return raw_hash;
    }

    size_t table_size() const { return data.md.size(); }

    double fill_ratio() const { return double(num_entries) / table_size(); }
    static constexpr double MAX_LOAD = 0.7;

    MapData data;
    size_t salt;
    size_t num_entries;
    size_t mod_mask;
    int32_t size_in_powers_of_two;
};

template<typename Key, typename Value> struct KeyValue {
    Key *key;
    Value *value;
};

template<typename Key, typename Value> class HashMapIterator final {
public:
    HashMapIterator(HashMap<Key, Value> *map, size_t offset) : map{map}, offset{offset} {
        if(offset == 0 && !map->data.has_value_at(offset)) {
            advance();
        }
    }

    KeyValue<Key, Value> operator*() {
        return KeyValue{map->data.keyptr(offset), map->data.valueptr(offset)};
    }

    bool operator!=(const HashMapIterator<Key, Value> &o) const { return offset != o.offset; };

    HashMapIterator<Key, Value> &operator++() {
        advance();
        return *this;
    }

private:
    void advance() {
        ++offset;
        while(offset < map->table_size() && !map->data.has_value_at(offset)) {
            ++offset;
        }
    }

    HashMap<Key, Value> *map;
    size_t offset;
};

template<WellBehaved Key> class HashSetIterator final {
public:
    HashSetIterator(HashMapIterator<Key, int> it_) : it{it_} {}

    Key &operator*() { return *(*it).key; }

    bool operator!=(const HashSetIterator<Key> &o) const { return it != o.it; };

    HashSetIterator<Key> &operator++() {
        ++it;
        return *this;
    }

private:
    HashMapIterator<Key, int> it;
};

struct HashInsertResult {
    int first_; // FIXME, should be an iterator.
    bool second;
};

template<WellBehaved Key, WellBehaved HashAlgo = SimpleHash> class HashSet final {

public:
    HashInsertResult insert(const Key &key) {
        // FIXME, inefficient. Does the lookup twice.
        bool inserted = !contains(key);
        map.insert(key, 1);
        return HashInsertResult{42, inserted};
    }

    bool contains(const Key &key) const { return map.contains(key); }

    void remove(const Key &k) { map.remove(k); }

    size_t size() const noexcept { return map.size(); }

    bool is_empty() const noexcept { return size() == 0; }

    void clear() noexcept { map.clear(); }

    HashSetIterator<Key> begin() const { return HashSetIterator<Key>(map.begin()); }

    HashSetIterator<Key> end() const { return HashSetIterator<Key>(map.end()); }

private:
    // This is inefficient.
    // It is just to get the implementation going
    // and work out the API.
    // Replace with a proper implementation later.
    HashMap<Key, int, HashAlgo> map;
};

class Range {
public:
    Range() noexcept : Range(0) {};
    Range(Range &&o) noexcept = default;
    explicit Range(int64_t end);
    Range(int64_t start, int64_t end);
    Range(int64_t start, int64_t end, int64_t step);

    Optional<int64_t> next();

    Range &operator=(const Range &o) noexcept = default;

private:
    int64_t i;
    int64_t end;
    int64_t step;
};

class GlobResult;

class Path {
public:
    Path() noexcept {};
    Path(const char *path);
    explicit Path(CString path);

    bool exists() const noexcept;
    bool is_file() const noexcept;
    bool is_dir() const noexcept;

    bool is_abs() const;

    CString extension() const;
    Path filename() const;

    Vector<CString> split() const;

    Path operator/(const Path &o) const noexcept;
    Path operator/(const char *str) const noexcept;
    Path operator/(const CString &str) const noexcept;

    Optional<Bytes> load_bytes();
    Optional<U8String> load_text();

    void replace_extension(CStringView new_extension);
    void replace_extension(const char *str) { replace_extension(CStringView(str)); }

    const char *c_str() const noexcept { return buf.c_str(); }
    size_t size() const noexcept { return buf.size(); }

    bool is_empty() const noexcept { return buf.is_empty(); }

    bool rename_to(const Path &targetname) const noexcept;

    GlobResult glob(const char *pattern);

private:
    CString buf;
};

class GlobResultInternal;

class GlobResult {
public:
    friend class Path;
    GlobResult() noexcept;
    GlobResult(GlobResult &&o) noexcept;
    ~GlobResult();
    Optional<Path> next();

    GlobResult &operator=(GlobResult &&o) noexcept = default;

private:
    GlobResult(const Path &path, const char *glob_pattern);

    unique_ptr<GlobResultInternal> p;
};

class MMapping {
public:
    MMapping() noexcept : buf{nullptr}, bufsize{0} {};
    MMapping(void *buf_, size_t bufsize_) : buf{buf_}, bufsize(bufsize_) {}
    MMapping(const MMapping &) = delete;
    MMapping(MMapping &&o) noexcept : buf{o.buf}, bufsize{o.bufsize} {
        o.buf = nullptr;
        o.bufsize = 0;
    }
    ~MMapping();

    BytesView view() const { return BytesView{(const char *)buf, bufsize}; }

    MMapping &operator=(MMapping &&o) noexcept {
        if(this != &o) {
            buf = o.buf;
            bufsize = o.bufsize;
            o.buf = nullptr;
            o.bufsize = 0;
        }
        return *this;
    }

private:
    void *buf;
    size_t bufsize;
};

Optional<MMapping> mmap_file(const char *path);

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
        if constexpr(index < sizeof...(T)) {
            using curtype = T...[index];
            reinterpret_cast<curtype *>(buf)->~curtype();
        }
        type_id = -1;
    }

    alignas(compute_alignment<0, T...>()) char buf[compute_size<0, T...>()];
    int8_t type_id;
};

template<typename T> class Stack {
public:
    Stack() = default;
    Stack(Stack &&o) = default;

    Stack &operator=(Stack &&o) noexcept = default;

    bool is_empty() const { return backing.is_empty(); }

    T &top() { return backing.back(); }

    const T &top() const { return backing.back(); }

    size_t size() const noexcept { return backing.size(); }

    void pop() { backing.pop_back(); }

    void push(const T &obj) { backing.push_back(obj); }

    void clear() noexcept { backing.clear(); }

private:
    Vector<T> backing;
};

typedef void (*StringCFormatCallback)(const char *buf, size_t bufsize, void *ctx);
CString cformat(const char *format, ...);
void cformat_with_cb(StringCFormatCallback cb, void *ctx, const char *format, va_list ap);

template<typename T> void format_append(T &oobj, const char *format, ...) {
    va_list args;
    va_start(args, format);
    auto converter = [](const char *buf, size_t bufsize, void *ctx) {
        auto *s = reinterpret_cast<T *>(ctx);
        if constexpr(is_same_v<T, CString>) {
            *s += buf;
        } else {
            for(size_t i = 0; i < bufsize; ++i) {
                s->push_back(buf[i]);
            }
        }
    };
    cformat_with_cb(converter, &oobj, format, args);
    va_end(args);
}

// Algorithms here

template<WellBehaved It1, WellBehaved It2, typename Value>
It1 find(It1 start, It2 end, const Value v) {
    while(start != end) {
        if(*start == v) {
            return start;
        }
        ++start;
    }
    return start;
}

template<WellBehaved It1, WellBehaved It2, typename Callable>
It1 find_if(It1 start, It2 end, const Callable c) {
    while(start != end) {
        if(c(*start)) {
            return start;
        }
        ++start;
    }
    return start;
}

template<WellBehaved T> void swap(T &a, T &b) noexcept {
    if(&a != &b) {
        T tmp{pystd2026::move(a)};
        a = pystd2026::move(b);
        b = pystd2026::move(tmp);
    }
}

template<BasicIterator It1, BasicIterator It2> It1 min_element(It1 start, It2 stop) noexcept {
    if(start == stop) {
        return start;
    }
    It1 minloc = start;
    while(start != stop) {
        if(*start < *minloc) {
            minloc = start;
        }
        ++start;
    }
    return minloc;
}

template<BasicIterator It1, BasicIterator It2> void insertion_sort(It1 start, It2 end) {
    const auto array_size = end - start;
    if(array_size < 2) {
        return;
    }
    if(array_size == 2) {
        auto first = start;
        auto second = first;
        ++second;
        if(!(*first < *second)) {
            swap(*first, *second);
        }
        return;
    }
    auto min_loc = min_element(start, end);
    swap(*min_loc, *start);
    ++start;
    ++start;
    while(start != end) {
        It1 current = start;
        auto previous = current;
        --previous;
        while(*current < *previous) {
            swap(*previous, *current);
            --previous;
            --current;
        }
        ++start;
    }
}

template<typename T> void sort_relocatable(T *data, size_t bufsize) {
    auto ordering = [](const void *v1, const void *v2) -> int {
        auto d1 = (T *)v1;
        auto d2 = (T *)v2;
        return *d1 <=> *d2;
    };
    qsort(data, bufsize, sizeof(T), ordering);
}

template<BasicIterator It, typename Value, typename Callable>
It lower_bound(It first, It last, const Value &value, const Callable &is_less) {
    It it{};
    auto count = last - first;
    while(count > 0) {
        it = first;
        auto step = count / 2;
        it += step;
        if(is_less(*it, value)) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return it;
}

template<BasicIterator It, typename Value> It lower_bound(It first, It last, const Value &value) {
    return lower_bound(
        first, last, value, [](const Value &v1, const Value &v2) { return v1 < v2; });
}

template<BasicIterator It, class Predicate> It find_if(It first, It last, const Predicate &&pred) {
    for(; first != last; ++first)
        if(pred(*first))
            return first;
    return last;
}

template<BasicIterator It, class Predicate>
It find_if_not(It first, It last, const Predicate &&pred) {
    for(; first != last; ++first)
        if(!pred(*first))
            return first;
    return last;
}

template<BasicIterator It, typename Predicate>
It partition(It first, It last, const Predicate &&pred) {
    first = find_if_not(first, last, move(pred));
    if(first == last)
        return first;

    It i = first;
    ++i;
    for(; i != last; ++i) {
        if(pred(*i)) {
            swap(*i, *first);
            ++first;
        }
    }
    return first;
}

double clamp(double val, double lower, double upper);
int64_t clamp(int64_t val, int64_t lower, int64_t upper);

struct UnicodeConversionResult {
    uint32_t codepoints[3];
};

UnicodeConversionResult uppercase_unicode(uint32_t codepoint);
UnicodeConversionResult lowercase_unicode(uint32_t codepoint);

} // namespace pystd2026
