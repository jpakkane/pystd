// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#ifndef __GLIBCXX__
void *operator new(size_t, void *ptr) noexcept;
#endif

namespace pystd2025 {

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

template<class T> struct is_const : pystd2025::false_type {};
template<class T> struct is_const<const T> : pystd2025::true_type {};
template<class T> constexpr bool is_const_v = is_const<T>::value;

template<class T> struct is_volatile : pystd2025::false_type {};
template<class T> struct is_volatile<volatile T> : pystd2025::true_type {};
template<class T> constexpr bool is_volatile_v = is_volatile<T>::value;

template<class T> struct is_lvalue_reference : pystd2025::false_type {};
template<class T> struct is_lvalue_reference<T &> : pystd2025::true_type {};

template<typename T>
constexpr T &&forward(typename pystd2025::remove_reference<T>::type &t) noexcept {
    return static_cast<T &&>(t);
}

template<typename T>
constexpr T &&forward(typename pystd2025::remove_reference<T>::type &&t) noexcept {
    static_assert(!pystd2025::is_lvalue_reference<T>::value,
                  "Forward may not be used to convert an rvalue to an lvalue");
    return static_cast<T &&>(t);
}

[[noreturn]] void internal_failure(const char *message) noexcept;

// This is still WIP. But basically require that objects of the
// given type can be default constructed and moved without exceptions.
template<typename T>
concept WellBehaved = requires(T a, T &b, T &&c) {
    requires noexcept(a = ::pystd2025::move(a));
    requires noexcept(a = ::pystd2025::move(b));
    requires noexcept(b = ::pystd2025::move(a));
    requires noexcept(a = ::pystd2025::move(c));
    requires noexcept(T{});
    requires noexcept(T{::pystd2025::move(a)});
    requires noexcept(T{::pystd2025::move(b)});
    requires noexcept(T{::pystd2025::move(c)});
    requires !is_volatile_v<T>;
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
    Unexpected(E &&e_) noexcept : e{pystd2025::move(e_)} {}
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
        new(content) E(pystd2025::move(e.error()));
    }

    Expected(const V &v) noexcept : state{UnionState::Value} { new(content) V(v); }

    Expected(V &&v) noexcept : state{UnionState::Value} { new(content) V(pystd2025::move(v)); }

    Expected(const E &e) noexcept : state{UnionState::Error} { new(content) E(e); }
    Expected(E &&e) noexcept : state{UnionState::Error} { new(content) E(pystd2025::move(e)); }

    Expected(Expected<V, E> &&o) noexcept : state{o.state} {
        if(o.has_value()) {
            new(content) V(pystd2025::move(o.value()));
        } else {
            new(content) E(pystd2025::move(o.error()));
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
                new(content) V(pystd2025::move(*reinterpret_cast<V *>(content)));
                state = UnionState::Value;
            } else if(o.has_error()) {
                new(content) E(pystd2025::move(*reinterpret_cast<E *>(o.content)));
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
    char content[maxval(sizeof(V), sizeof(E))] alignas(maxval(alignof(V), alignof(E)));
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

private:
    T *ptr;
};

template<typename T> class unique_arr final {
public:
    unique_arr() noexcept : ptr{nullptr} {}
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
    T *ptr;
    size_t array_size;
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
                new(&data.value) T{o.data.value};
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
            data.value = pystd2025::move(o);
        } else {
            new(&data.value) T{o};
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

private:
    void grow_to(size_t new_size);

    unique_arr<char> buf; // Not zero terminated.
    size_t bufsize;
};

template<WellBehaved T> class Vector final {

public:
    Vector() noexcept = default;

    Vector(const Vector<T> &o) {
        reserve(o.size());
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

    Vector(Vector<T> &&o) noexcept : backing(move(o.backing)), num_entries{o.num_entries} {
        o.num_entries = 0;
    }

    ~Vector() {
        for(size_t i = 0; i < num_entries; ++i) {
            objptr(i)->~T();
        }
    }

    void push_back(const T &obj) noexcept {
        if(is_ptr_within(&obj) && needs_to_grow_for(1)) {
            // Fixme, maybe compute index to the backing store
            // and then use that, skipping the temporary.
            T tmp{obj};
            backing.extend(sizeof(T));
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(::pystd2025::move(tmp));
            ++num_entries;
        } else {
            backing.extend(sizeof(T));
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(obj);
            ++num_entries;
        }
    }

    void push_back(T &&obj) noexcept {
        if(is_ptr_within(&obj) && needs_to_grow_for(1)) {
            // Fixme, maybe compute index to the backing store
            // and then use that, skipping the temporary.
            T tmp{::pystd2025::move(obj)};
            backing.extend(sizeof(T));
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(::pystd2025::move(tmp));
            ++num_entries;
        } else {
            backing.extend(sizeof(T));
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(::pystd2025::move(obj));
            ++num_entries;
        }
    }

    void emplace_back(auto &&...args) noexcept {
        if constexpr(sizeof...(args) == 1 && pystd2025::is_same_v<decltype(args...[0]), T>) {
            this->push_back(pystd2025::forward(args...[0]));
        } else {
            backing.extend(sizeof(T));
            auto obj_loc = objptr(num_entries);
            new(obj_loc) T(pystd2025::forward<decltype(args)>(args)...);
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

    void pop_back() noexcept {
        if(num_entries == 0) {
            return;
        }
        T *obj = objptr(num_entries - 1);
        obj->~T();
        backing.shrink(sizeof(T));
        --num_entries;
    }

    size_t capacity() const noexcept { return backing.size() / sizeof(T); }
    size_t size() const noexcept { return num_entries; }

    bool is_empty() const noexcept { return size() == 0; }

    void clear() noexcept {
        while(!is_empty()) {
            pop_back();
        }
    }

    T &front() {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[0];
    }

    const T &front() const {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[0];
    }

    T &back() {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    const T &back() const {
        if(is_empty()) {
            throw PyException("Tried to access empty array.");
        }
        return (*this)[size() - 1];
    }

    T *data() { return (T *)backing.data(); }

    T &operator[](size_t i) {
        if(i >= num_entries) {
            throw PyException("Vector index out of bounds.");
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= num_entries) {
            throw PyException("Vector index out of bounds.");
        }
        return *objptr(i);
    }

    T &at(size_t i) { return (*this)[i]; }

    const T &at(size_t i) const { return (*this)[i]; }

    Vector<T> &operator=(Vector<T> &&o) noexcept {
        if(this != &o) {
            backing = move(o.backing);
            num_entries = o.num_entries;
            o.num_entries = 0;
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
            if(backing[i] != o.backing[i]) {
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
            backing.resize_to(new_size * sizeof(T));
        }
    }

private:
    T *objptr(size_t i) noexcept { return reinterpret_cast<T *>(rawptr(i)); }
    const T *objptr(size_t i) const noexcept { return reinterpret_cast<const T *>(rawptr(i)); }
    char *rawptr(size_t i) noexcept { return backing.data() + i * sizeof(T); }
    const char *rawptr(size_t i) const noexcept { return backing.data() + i * sizeof(T); }

    bool needs_to_grow_for(size_t num_new_items) {
        return num_entries + num_new_items * sizeof(T) > backing.size();
    }

    bool is_ptr_within(const T *ptr) const { return backing.is_ptr_within((const char *)ptr); }

    Bytes backing;
    size_t num_entries = 0;
};

// Iterates over the code points of a valid UTF 8 string.
// If the string used is not valid UTF-8, result is undefined.
class ValidatedU8Iterator {
public:
    friend class U8String;
    friend class U8Match;
    friend class U8StringView;

    ValidatedU8Iterator(const ValidatedU8Iterator &) = default;
    ValidatedU8Iterator(ValidatedU8Iterator &&) = default;

    uint32_t operator*();

    ValidatedU8Iterator &operator++();
    ValidatedU8Iterator operator++(int);

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

    const unsigned char *buf;
    CharInfo char_info;
    bool has_char_info;
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

    bool operator==(const char *str);
    bool operator==(CStringView o);
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

    char at(size_t index) const { return (*this)[index]; }

    const char *begin() const { return buf; }

    const char *end() const { return buf + bufsize; }

    bool operator<(const CStringView &o) const;

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

    void operator=(const pystd2025::CString &) noexcept;
    void operator=(CString &&o) noexcept {
        if(this != &o) {
            bytes = move(o.bytes);
        }
    }

    bool operator==(const CString &o) const { return bytes == o.bytes; }
    bool operator<(const CString &o) const { return bytes < o.bytes; }
    // Fixme: add <=>

    bool operator==(const char *str);
    bool operator==(const CStringView &o);

    CString &operator+=(const CString &o);

    CString &operator+=(char c);

    CString &operator+=(const char *str);

    CString &operator=(const char *);

    void append(const char c);
    void append(const char *str, size_t strsize = -1);
    void append(const char *start, const char *stop);

    template<typename Hasher> void feed_hash(Hasher &h) const { bytes.feed_hash(h); }

    template<typename T = CString> Vector<T> split() const;

    void split(CStringViewCallback cb, void *ctx) const;

    bool is_empty() const {
        // The buffer always has a null terminator.
        return bytes.size() == 1;
    }

    char front() const { return bytes.front(); }

    char back() const;

    void pop_back() noexcept;
    void push_back(const char c) noexcept { append(c); }

    void insert(size_t i, const CStringView &v) noexcept;

    size_t rfind(const char c) const noexcept;

    void reserve(size_t new_size) noexcept { bytes.reserve(new_size); }

    void resize(size_t new_size) noexcept { bytes.resize(new_size); }

private:
    bool view_points_to_this(const CStringView &v) const;

    void check_embedded_nuls() const;

    Bytes bytes;
};

struct U8StringView {
    ValidatedU8Iterator start;
    ValidatedU8Iterator end;

    CStringView raw_view() const;

    bool operator==(const char *) const;
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
        (*this) = pystd2025::move(tmp);
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
    Span(const T *src, size_t src_size) noexcept : array(src), arraysize(src_size) {}
    Span(Span &&o) noexcept = default;
    Span(const Span &o) noexcept = default;

    Span &operator=(Span &&o) noexcept = default;
    Span &operator=(const Span &o) noexcept = default;

    const T &operator[](size_t i) const {
        if(i > arraysize) {
            throw PyException("OOB in span.");
        }
        return array[i];
    }

    const T &at(size_t i) const { return (*this)[i]; }

    const T *begin() const { return array; }

    const T *end() const { return array + arraysize; }

    size_t size() const { return arraysize; }

private:
    const T *array;
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
        data.hashes = unique_arr<size_t>(initial_table_size);
        data.reset_hash_values();
        data.keydata = Bytes(initial_table_size * sizeof(Key));
        data.valuedata = Bytes(initial_table_size * sizeof(Value));
    }

    Value *lookup(const Key &key) const {
        const auto hashval = hash_for(key);
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.hashes[slot] == FREE_SLOT) {
                return nullptr;
            }
            if(data.hashes[slot] == hashval) {
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

    void insert(const Key &key, Value v) {
        if(fill_ratio() >= MAX_LOAD) {
            grow();
        }

        const auto hashval = hash_for(key);
        insert_internal(hashval, key, pystd2025::move(v));
    }

    void remove(const Key &key) {
        const auto hashval = hash_for(key);
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.hashes[slot] == FREE_SLOT) {
                return;
            } else if(data.hashes[slot] == TOMBSTONE) {

            } else if(data.hashes[slot] == hashval) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    auto *value_loc = data.valueptr(slot);
                    value_loc->~Value();
                    const auto previous_slot = (slot + table_size() - 1) & mod_mask;
                    const auto next_slot = (slot + 1) % mod_mask;
                    if(data.hashes[previous_slot] == FREE_SLOT &&
                       data.hashes[next_slot] == FREE_SLOT) {
                        data.hashes[slot] = FREE_SLOT;
                    } else {
                        data.hashes[slot] = TOMBSTONE;
                    }
                    --num_entries;
                    return;
                }
            }
            slot = (slot + 1) & mod_mask;
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
    // This is neither fast, memory efficient nor elegant.
    // At this point in the project life cycle it does not need to be.
    struct MapData {
        unique_arr<size_t> hashes;
        Bytes keydata;
        Bytes valuedata;

        MapData() = default;
        MapData(MapData &&o) noexcept {
            hashes = move(o.hashes);
            keydata = move(o.keydata);
            valuedata = move(o.valuedata);
        }

        void operator=(MapData &&o) noexcept {
            if(this != &o) {
                hashes = move(o.hashes);
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

        void reset_hash_values() noexcept {
            for(size_t i = 0; i < hashes.size(); ++i) {
                hashes[i] = FREE_SLOT;
            }
        }

        void deallocate_contents() noexcept {
            for(size_t i = 0; i < hashes.size(); ++i) {
                if(!has_value(i)) {
                    continue;
                }
                keyptr(i)->~Key();
                valueptr(i)->~Value();
            }
        }

        void clear() noexcept {
            deallocate_contents();
            reset_hash_values();
        }

        bool has_value(size_t offset) const {
            if(offset >= hashes.size()) {
                return false;
            }
            return !(hashes[offset] == TOMBSTONE || hashes[offset] == FREE_SLOT);
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

    void insert_internal(size_t hashval, const Key &key, Value &&v) {
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.hashes[slot] == FREE_SLOT) {
                auto *key_loc = data.keyptr(slot);
                auto *value_loc = data.valueptr(slot);
                new(key_loc) Key(key);
                new(value_loc) Value{pystd2025::move(v)};
                data.hashes[slot] = hashval;
                ++num_entries;
                return;
            }
            if(data.hashes[slot] == hashval) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    auto *value_loc = data.valueptr(slot);
                    *value_loc = pystd2025::move(v);
                    return;
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

        grown.hashes = unique_arr<size_t>(new_size);
        grown.keydata = Bytes(new_size * sizeof(Key));
        grown.valuedata = Bytes(new_size * sizeof(Value));

        grown.reset_hash_values();
        MapData old = move(data);
        data = move(grown);
        size_in_powers_of_two = new_powers_of_two;
        mod_mask = new_mod_mask;
        num_entries = 0;
        for(size_t i = 0; i < old.hashes.size(); ++i) {
            if(old.hashes[i] == FREE_SLOT || old.hashes[i] == TOMBSTONE) {
                continue;
            }
            insert_internal(old.hashes[i], *old.keyptr(i), pystd2025::move(*old.valueptr(i)));
        }
    }

    size_t hash_for(const Key &k) const {
        Hasher<HashAlgo> h;
        h.feed_hash(salt);
        h.feed_hash(k);
        auto raw_hash = h.get_hash_value();
        if(raw_hash == FREE_SLOT) {
            raw_hash = FREE_SLOT + 1;
        } else if(raw_hash == TOMBSTONE) {
            raw_hash = TOMBSTONE + 1;
        }
        return raw_hash;
    }

    size_t table_size() const { return data.hashes.size(); }

    double fill_ratio() const { return double(num_entries) / table_size(); }
    static constexpr size_t FREE_SLOT = 42;
    static constexpr size_t TOMBSTONE = 666;
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
        if(offset == 0 && !map->data.has_value(offset)) {
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
        while(offset < map->table_size() && !map->data.has_value(offset)) {
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

class U8Regex {
public:
    explicit U8Regex(const U8String &pattern);
    ~U8Regex();

    U8Regex() = delete;
    U8Regex(const U8Regex &o) = delete;
    U8Regex &operator=(const U8Regex &o) = delete;

    void *h() const { return handle; }

private:
    void *handle;
};

class U8Match {
public:
    U8Match(void *h, const U8String *orig) : handle{h}, original{orig} {};
    ~U8Match();

    U8Match() = delete;
    U8Match(U8Match &&o) : handle(o.handle), original{o.original} {
        o.handle = nullptr;
        o.original = nullptr;
    }
    U8Match &operator=(const U8Match &o) = delete;

    void *h() const { return handle; }

    U8String get_submatch(size_t i);

    size_t num_groups() const;

    U8StringView group(size_t group_num) const;

private:
    void *handle;
    const U8String *original; // Not very safe...
};

U8Match regex_search(const U8Regex &pattern, const U8String &text);

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

    Path operator/(const Path &o) const noexcept;
    Path operator/(const char *str) const noexcept;

    Optional<Bytes> load_bytes();
    Optional<U8String> load_text();

    void replace_extension(CStringView new_extension);
    void replace_extension(const char *str) { replace_extension(CStringView(str)); }

    const char *c_str() const noexcept { return buf.c_str(); }
    size_t size() const noexcept { return buf.size(); }

    bool is_empty() const noexcept { return buf.is_empty(); }

    bool rename_to(const Path &targetname) const noexcept;

private:
    CString buf;
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
    Variant(Variant<T...> &&o) noexcept { move_to_uninitialized_memory(pystd2025::move(o)); }

    int8_t index() const noexcept { return type_id; }

#define PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(i)                                                     \
    {                                                                                              \
        if constexpr(i == new_type) {                                                              \
            new(buf) T... [i] { pystd2025::move(o) };                                              \
        }                                                                                          \
    }                                                                                              \
    break;

    template<typename InType> Variant(InType &&o) noexcept {
        constexpr int new_type = get_index_for_type<InType>();
        switch(new_type) {
        case 0:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(0);
        case 1:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(1);
        case 2:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(2);
        case 3:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(3);
        case 4:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(4);
        case 5:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(5);
        case 6:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(6);
        case 7:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(7);
        case 8:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(8);
        case 9:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(9);
        case 10:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(10);
        case 11:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(11);
        case 12:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(12);
        case 13:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(13);
        case 14:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(14);
        case 15:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(15);
        case 16:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(16);
        case 17:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(17);
        case 18:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(18);
        case 19:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(19);
        case 20:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(20);
        case 21:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(21);
        case 22:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(22);
        case 23:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(23);
        case 24:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(24);
        case 25:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(25);
        case 26:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(26);
        case 27:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(27);
        case 28:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(28);
        case 29:
            PYSTD2025_VAR_MOVE_CONSTRUCT_SWITCH(29);
        default:
            internal_failure("Bad variant construction.");
        }

        type_id = new_type;
    }

#define PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(i)                                                     \
    {                                                                                              \
        if constexpr(i == new_type) {                                                              \
            new(buf) T...[i](o);                                                                   \
        }                                                                                          \
    }                                                                                              \
    break;

    template<typename InType> Variant(InType &o) {
        constexpr int new_type = get_index_for_type<InType>();
        switch(new_type) {
        case 0:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(0);
        case 1:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(1);
        case 2:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(2);
        case 3:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(3);
        case 4:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(4);
        case 5:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(5);
        case 6:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(6);
        case 7:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(7);
        case 8:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(8);
        case 9:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(9);
        case 10:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(10);
        case 11:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(11);
        case 12:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(12);
        case 13:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(13);
        case 14:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(14);
        case 15:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(15);
        case 16:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(16);
        case 17:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(17);
        case 18:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(18);
        case 19:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(19);
        case 20:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(20);
        case 21:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(21);
        case 22:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(22);
        case 23:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(23);
        case 24:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(24);
        case 25:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(25);
        case 26:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(26);
        case 27:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(27);
        case 28:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(28);
        case 29:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(29);
        default:
            internal_failure("Bad variant construction.");
        }

        type_id = new_type;
    }

    template<typename InType> Variant(const InType &o) {
        constexpr int new_type = get_index_for_type<InType>();
        switch(new_type) {
        case 0:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(0);
        case 1:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(1);
        case 2:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(2);
        case 3:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(3);
        case 4:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(4);
        case 5:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(5);
        case 6:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(6);
        case 7:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(7);
        case 8:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(8);
        case 9:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(9);
        case 10:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(10);
        case 11:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(11);
        case 12:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(12);
        case 13:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(13);
        case 14:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(14);
        case 15:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(15);
        case 16:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(16);
        case 17:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(17);
        case 18:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(18);
        case 19:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(19);
        case 20:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(20);
        case 21:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(21);
        case 22:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(22);
        case 23:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(23);
        case 24:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(24);
        case 25:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(25);
        case 26:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(26);
        case 27:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(27);
        case 28:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(28);
        case 29:
            PYSTD2025_VAR_COPY_CONSTRUCT_SWITCH(29);
        default:
            internal_failure("Bad variant construction.");
        }

        type_id = new_type;
    }

    ~Variant() { destroy(); }

    template<WellBehaved Q> bool contains() const noexcept {
        const auto id = get_index_for_type<Q>();
        return id == type_id;
    }

    template<typename Desired> Desired *get_if() noexcept {
        const int computed_type = get_index_for_type<Desired>();
        if(computed_type == type_id) {
            return reinterpret_cast<Desired *>(buf);
        }
        return nullptr;
    }

    template<typename Desired> const Desired *get_if() const noexcept {
        const int computed_type = get_index_for_type<Desired>();
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
        constexpr int new_type = get_index_for_type<OBJ>();
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
#define PYSTD2025_VAR_MOVE_SWITCH(i)                                                               \
    {                                                                                              \
        if constexpr(i < sizeof...(T)) {                                                           \
            new(buf) T... [i] { move(o.get<T...[i]>()) };                                          \
        }                                                                                          \
    }                                                                                              \
    break;

    void move_to_uninitialized_memory(Variant<T...> &&o) noexcept {
        switch(o.type_id) {
        case 0:
            PYSTD2025_VAR_MOVE_SWITCH(0);
        case 1:
            PYSTD2025_VAR_MOVE_SWITCH(1);
        case 2:
            PYSTD2025_VAR_MOVE_SWITCH(2);
        case 3:
            PYSTD2025_VAR_MOVE_SWITCH(3);
        case 4:
            PYSTD2025_VAR_MOVE_SWITCH(4);
        case 5:
            PYSTD2025_VAR_MOVE_SWITCH(5);
        case 6:
            PYSTD2025_VAR_MOVE_SWITCH(6);
        case 7:
            PYSTD2025_VAR_MOVE_SWITCH(7);
        case 8:
            PYSTD2025_VAR_MOVE_SWITCH(8);
        case 9:
            PYSTD2025_VAR_MOVE_SWITCH(9);
        case 10:
            PYSTD2025_VAR_MOVE_SWITCH(10);
        case 11:
            PYSTD2025_VAR_MOVE_SWITCH(11);
        case 12:
            PYSTD2025_VAR_MOVE_SWITCH(12);
        case 13:
            PYSTD2025_VAR_MOVE_SWITCH(13);
        case 14:
            PYSTD2025_VAR_MOVE_SWITCH(14);
        case 15:
            PYSTD2025_VAR_MOVE_SWITCH(15);
        case 16:
            PYSTD2025_VAR_MOVE_SWITCH(16);
        case 17:
            PYSTD2025_VAR_MOVE_SWITCH(17);
        case 18:
            PYSTD2025_VAR_MOVE_SWITCH(18);
        case 19:
            PYSTD2025_VAR_MOVE_SWITCH(19);
        case 20:
            PYSTD2025_VAR_MOVE_SWITCH(20);
        case 21:
            PYSTD2025_VAR_MOVE_SWITCH(21);
        case 22:
            PYSTD2025_VAR_MOVE_SWITCH(22);
        case 23:
            PYSTD2025_VAR_MOVE_SWITCH(23);
        case 24:
            PYSTD2025_VAR_MOVE_SWITCH(24);
        case 25:
            PYSTD2025_VAR_MOVE_SWITCH(25);
        case 26:
            PYSTD2025_VAR_MOVE_SWITCH(26);
        case 27:
            PYSTD2025_VAR_MOVE_SWITCH(27);
        case 28:
            PYSTD2025_VAR_MOVE_SWITCH(28);
        case 29:
            PYSTD2025_VAR_MOVE_SWITCH(29);
        default:
            internal_failure("Unreachable code in variant move.");
        }
        type_id = o.type_id;
    }

// FIXME, update to do a constexpr check to see if the
// value can be nothrow copied.
#define PYSTD2025_VAR_COPY_SWITCH(i)                                                               \
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
            PYSTD2025_VAR_COPY_SWITCH(0);
        case 1:
            PYSTD2025_VAR_COPY_SWITCH(1);
        case 2:
            PYSTD2025_VAR_COPY_SWITCH(2);
        case 3:
            PYSTD2025_VAR_COPY_SWITCH(3);
        case 4:
            PYSTD2025_VAR_COPY_SWITCH(4);
        case 5:
            PYSTD2025_VAR_COPY_SWITCH(5);
        case 6:
            PYSTD2025_VAR_COPY_SWITCH(6);
        case 7:
            PYSTD2025_VAR_COPY_SWITCH(7);
        case 8:
            PYSTD2025_VAR_COPY_SWITCH(8);
        case 9:
            PYSTD2025_VAR_COPY_SWITCH(9);
        case 10:
            PYSTD2025_VAR_COPY_SWITCH(10);
        case 11:
            PYSTD2025_VAR_COPY_SWITCH(11);
        case 12:
            PYSTD2025_VAR_COPY_SWITCH(12);
        case 13:
            PYSTD2025_VAR_COPY_SWITCH(13);
        case 14:
            PYSTD2025_VAR_COPY_SWITCH(14);
        case 15:
            PYSTD2025_VAR_COPY_SWITCH(15);
        case 16:
            PYSTD2025_VAR_COPY_SWITCH(16);
        case 17:
            PYSTD2025_VAR_COPY_SWITCH(17);
        case 18:
            PYSTD2025_VAR_COPY_SWITCH(18);
        case 19:
            PYSTD2025_VAR_COPY_SWITCH(19);
        case 20:
            PYSTD2025_VAR_COPY_SWITCH(20);
        case 21:
            PYSTD2025_VAR_COPY_SWITCH(21);
        case 22:
            PYSTD2025_VAR_COPY_SWITCH(22);
        case 23:
            PYSTD2025_VAR_COPY_SWITCH(23);
        case 24:
            PYSTD2025_VAR_COPY_SWITCH(24);
        case 25:
            PYSTD2025_VAR_COPY_SWITCH(25);
        case 26:
            PYSTD2025_VAR_COPY_SWITCH(26);
        case 27:
            PYSTD2025_VAR_COPY_SWITCH(27);
        case 28:
            PYSTD2025_VAR_COPY_SWITCH(28);
        case 29:
            PYSTD2025_VAR_COPY_SWITCH(29);
        default:
            internal_failure("Unreachable code in variant copy.");
        }

        type_id = o.type_id;
    }

#define PYSTD2025_VAR_COMPARE_SWITCH(i)                                                            \
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
            PYSTD2025_VAR_COMPARE_SWITCH(0);
        case 1:
            PYSTD2025_VAR_COMPARE_SWITCH(1);
        case 2:
            PYSTD2025_VAR_COMPARE_SWITCH(2);
        case 3:
            PYSTD2025_VAR_COMPARE_SWITCH(3);
        case 4:
            PYSTD2025_VAR_COMPARE_SWITCH(4);
        case 5:
            PYSTD2025_VAR_COMPARE_SWITCH(5);
        case 6:
            PYSTD2025_VAR_COMPARE_SWITCH(6);
        case 7:
            PYSTD2025_VAR_COMPARE_SWITCH(7);
        case 8:
            PYSTD2025_VAR_COMPARE_SWITCH(8);
        case 9:
            PYSTD2025_VAR_COMPARE_SWITCH(9);
        case 10:
            PYSTD2025_VAR_COMPARE_SWITCH(10);
        case 11:
            PYSTD2025_VAR_COMPARE_SWITCH(11);
        case 12:
            PYSTD2025_VAR_COMPARE_SWITCH(12);
        case 13:
            PYSTD2025_VAR_COMPARE_SWITCH(13);
        case 14:
            PYSTD2025_VAR_COMPARE_SWITCH(14);
        case 15:
            PYSTD2025_VAR_COMPARE_SWITCH(15);
        case 16:
            PYSTD2025_VAR_COMPARE_SWITCH(16);
        case 17:
            PYSTD2025_VAR_COMPARE_SWITCH(17);
        case 18:
            PYSTD2025_VAR_COMPARE_SWITCH(18);
        case 19:
            PYSTD2025_VAR_COMPARE_SWITCH(19);
        case 20:
            PYSTD2025_VAR_COMPARE_SWITCH(20);
        case 21:
            PYSTD2025_VAR_COMPARE_SWITCH(21);
        case 22:
            PYSTD2025_VAR_COMPARE_SWITCH(22);
        case 23:
            PYSTD2025_VAR_COMPARE_SWITCH(23);
        case 24:
            PYSTD2025_VAR_COMPARE_SWITCH(24);
        case 25:
            PYSTD2025_VAR_COMPARE_SWITCH(25);
        case 26:
            PYSTD2025_VAR_COMPARE_SWITCH(26);
        case 27:
            PYSTD2025_VAR_COMPARE_SWITCH(27);
        case 28:
            PYSTD2025_VAR_COMPARE_SWITCH(28);
        case 29:
            PYSTD2025_VAR_COMPARE_SWITCH(29);
        }
        internal_failure("Unreachable code in variant compare.");
    }

    template<typename Desired, int8_t index> constexpr int8_t get_index_for_type() const noexcept {
        if constexpr(index >= sizeof...(T)) {
            static_assert(index < sizeof...(T));
        } else {
            if constexpr(is_same_v<Desired, T...[index]>) {
                return index;
            } else {
                return get_index_for_type<Desired, index + 1>();
            }
        }
    }

    template<typename Desired> constexpr int8_t get_index_for_type() const noexcept {
        return get_index_for_type<Desired, 0>();
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

    char buf[compute_size<0, T...>()] alignas(compute_alignment<0, T...>());
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
        T tmp{pystd2025::move(a)};
        a = pystd2025::move(b);
        b = pystd2025::move(tmp);
    }
}

template<typename It1, typename It2> It1 min_element(It1 start, It2 stop) noexcept {
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

template<typename It1, typename It2> void insertion_sort(It1 start, It2 end) {
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

template<typename It, typename Value> It lower_bound(It first, It last, const Value &value) {
    It it{};
    auto count = last - first;
    while(count > 0) {
        it = first;
        auto step = count / 2;
        it += step;
        if(*it < value) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    return it;
}

double clamp(double val, double lower, double upper);
int64_t clamp(int64_t val, int64_t lower, int64_t upper);

} // namespace pystd2025
