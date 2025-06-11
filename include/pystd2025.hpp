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

template<WellBehaved V, WellBehaved E> class Expected {
private:
    enum class UnionState : unsigned char {
        Empty,
        Value,
        Error,
    };

public:
    Expected() noexcept : state{UnionState::Empty} {}

    explicit Expected(const V &v) noexcept : state{UnionState::Value} { new(&content.value) V(v); }

    explicit Expected(const E &e) noexcept : state{UnionState::Error} { new(&content.error) E(e); }

    ~Expected() { destroy(); }

    bool has_value() const noexcept { return state == UnionState::Value; }

    bool has_error() const noexcept { return state == UnionState::Error; }

    operator bool() const noexcept { return has_value(); }

    void operator=(Expected<V, E> &&o) noexcept {
        if(this != &o) {
            destroy();
            if(o.has_value()) {
                new(&content.value) V(o.content.value);
                state = UnionState::Value;
            } else if(o.has_error()) {
                new(&content.error) E(o.content.error);
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
        return content.value;
    }

    E &error() {
        if(!has_error()) {
            abort();
        }
        return content.error;
    }

private:
    void destroy() {
        if(state == UnionState::Value) {
            content.value.~V();
        } else if(state == UnionState::Error) {
            content.error.~E();
        }
        state = UnionState::Empty;
    }
    union {
        V value;
        E error;
    } content;
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

template<typename T> class unique_ptr final {
public:
    unique_ptr() noexcept : ptr{nullptr} {}
    explicit unique_ptr(T *t) noexcept : ptr{t} {}
    explicit unique_ptr(const unique_ptr<T> &o) = delete;
    explicit unique_ptr(unique_ptr<T> &&o) noexcept : ptr{o.ptr} { o.ptr = nullptr; }
    ~unique_ptr() { delete ptr; }

    T &operator=(const unique_ptr<T> &o) = delete;

    T &operator=(unique_ptr<T> &&o) noexcept {
        if(this != &o) {
            delete ptr;
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        return *this;
    }

    T *get() noexcept { return ptr; }
    T &operator*() { return *ptr; } // FIXME, check not null maybe?
    T *operator->() noexcept { return ptr; }

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

    T &operator=(const unique_arr<T> &o) = delete;

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
            throw "Index out of bounds.";
        }
        return ptr[index];
    }

    const T &operator[](size_t index) const {
        if(index >= array_size) {
            throw "Index out of bounds.";
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
            data.value = o;
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
            throw "Empty optional.";
        }
        return data.value;
    }

    T *operator->() {
        if(!*this) {
            throw "Empty optional.";
        }
        return &data.value;
    }

    const T *operator->() const {
        if(!*this) {
            throw "Empty optional.";
        }
        return &data.value;
    }

    T &value() {
        if(!*this) {
            throw "Empty optional.";
        }
        return data.value;
    }

    const T &value() const {
        if(!*this) {
            throw "Empty optional.";
        }
        return data.value;
    }

    const T &operator*() const {
        if(!*this) {
            throw "Empty optional.";
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

class SimpleHasher final {
public:
    void feed_bytes(const char *buf, size_t bufsize) noexcept;

    template<typename T> void feed_primitive(const T &c) noexcept {
        // FIXME, add concept to only accept ints and floats.
        feed_bytes((const char *)&c, sizeof(c));
    }

    void feed(int8_t i) noexcept { feed_primitive(i); }

    void feed(uint8_t i) noexcept { feed_primitive(i); }

    void feed(int16_t i) noexcept { feed_primitive(i); }

    void feed(uint16_t i) noexcept { feed_primitive(i); }

    void feed(int32_t i) noexcept { feed_primitive(i); }

    void feed(uint32_t i) noexcept { feed_primitive(i); }

    void feed(int64_t i) noexcept { feed_primitive(i); }

    void feed(uint64_t i) noexcept { feed_primitive(i); }

    void feed(float i) noexcept { feed_primitive(i); }

    void feed(double i) noexcept { feed_primitive(i); }

    template<typename T> void feed(T *o) noexcept {
        static_assert(sizeof(void *) == 8);
        feed_primitive((uint64_t)o);
    }

    template<typename T> void feed(const T &o) noexcept { o.feed_hash(*this); }

    size_t get_hash() const { return value; }

private:
    size_t value{0};
};

class PyException;

class BytesView {
public:
    const char *buf;
    size_t start_offset;
    size_t end_offset;
};

class Bytes {
public:
    Bytes() noexcept;
    explicit Bytes(size_t initial_size) noexcept;
    Bytes(const char *buf, size_t bufsize) noexcept;
    Bytes(Bytes &&o) noexcept;
    Bytes(const Bytes &o) noexcept;
    const char *data() const { return buf.get(); }
    char *data() { return buf.get(); }

    void append(const char c);

    bool is_empty() const { return bufsize == 0; }
    void clear() noexcept { bufsize = 0; }

    size_t size() const { return bufsize; }

    size_t capacity() const { return buf.size_bytes(); }

    void extend(size_t num_bytes) noexcept;
    void shrink(size_t num_bytes) noexcept;
    void resize_to(size_t num_bytes) noexcept;

    void assign(const char *buf_in, size_t in_size);
    void insert(size_t i, const char *buf_in, size_t in_size);

    void pop_back(size_t num = 1);

    void pop_front(size_t num = 1);

    char operator[](size_t i) const { return buf[i]; }

    void operator=(const Bytes &) noexcept;

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
        for(const auto &i: o) {
            push_back(i);
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

    template<typename Iter1, typename Iter2>
    void append(Iter1 start, Iter2 end) {
        if(is_ptr_within(&(*start))) {
            throw "FIXME, appending contents of vector not handled yet.";
        }
        while(start != end) {
            push_back(*start);
            ++start;
        }
    }

    template<typename Iter1, typename Iter2>
    void assign(Iter1 start, Iter2 end) {
        if(is_ptr_within(&(*start))) {
            throw "FIXME, assigning subset of vector itself is not supported.";
        }
        clear();
        append(start, end);
    }

    void emplace_back(T &&obj) noexcept {
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
            throw "Index out of bounds.";
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= num_entries) {
            throw "Index out of bounds.";
        }
        return *objptr(i);
    }

    T &at(size_t i) {
        return (*this)[i];
    }

    const T &at(size_t i) const {
        return (*this)[i];
    }


    Vector<T> &operator=(Vector<T> &&o) noexcept {
        if(this != &o) {
            backing = move(o.backing);
            num_entries = o.num_entries;
            o.num_entries = 0;
        }
        return *this;
    }

    const T *cbegin() const { return objptr(0); }
    const T *cend() const { return objptr(num_entries); }

    T *begin() const { return const_cast<T *>(objptr(0)); }
    T *end() const { return const_cast<T *>(objptr(num_entries)); }

    void reserve(size_t new_size) {
        if(new_size > size()) {
            backing.resize_to(new_size*sizeof(T));
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

// No embedded null chars
// NOT guaranteed to be null terminated.
class CStringView {
public:
    CStringView() noexcept : buf{nullptr}, bufsize{0} {}
    explicit CStringView(const char *str) noexcept;
    CStringView(const char *str, size_t length);

    bool operator==(const char *str);
    bool is_empty() const { return bufsize == 0; }

    char front() const;
    bool starts_with(const char *str) const;

    const char *data() const { return buf; }
    size_t size() const { return bufsize; }
    size_t find(char c) const;
    CStringView substr(size_t pos = 0, size_t count = (size_t)-1) const;

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

    CStringView view() const;

    void strip();
    // CString stripped() const;

    size_t size() const;

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

    CString &operator+=(const CString &o);

    CString &operator+=(char c);

    CString &operator+=(const char *str);

    void append(const char c);

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

    U8String &operator=(const U8String &) noexcept;
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

template<typename Key, typename Value> class HashMapIterator;

template<WellBehaved Key, WellBehaved Value, typename Hasher = SimpleHasher> class HashMap final {
public:
    friend class HashMapIterator<Key, Value>;
    HashMap() {
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

    void insert(const Key &key, const Value &v) {
        if(fill_ratio() >= MAX_LOAD) {
            grow();
        }

        const auto hashval = hash_for(key);
        insert_internal(hashval, key, v);
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

    HashMapIterator<Key, Value> begin() { return HashMapIterator<Key, Value>(this, 0); }

    HashMapIterator<Key, Value> end() { return HashMapIterator<Key, Value>(this, table_size()); }

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

        void reset_hash_values() {
            for(size_t i = 0; i < hashes.size(); ++i) {
                hashes[i] = FREE_SLOT;
            }
        }

        void deallocate_contents() {
            for(size_t i = 0; i < hashes.size(); ++i) {
                if(!has_value(i)) {
                    continue;
                }
                keyptr(i)->~Key();
                valueptr(i)->~Value();
            }
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

    void insert_internal(size_t hashval, const Key &key, const Value &v) {
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.hashes[slot] == FREE_SLOT) {
                auto *key_loc = data.keyptr(slot);
                auto *value_loc = data.valueptr(slot);
                new(key_loc) Key(key);
                new(value_loc) Value{v};
                data.hashes[slot] = hashval;
                ++num_entries;
                return;
            }
            if(data.hashes[slot] == hashval) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    auto *value_loc = data.valueptr(slot);
                    *value_loc = v;
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
            insert_internal(old.hashes[i], *old.keyptr(i), *old.valueptr(i));
        }
    }

    size_t hash_for(const Key &k) const {
        Hasher h;
        h.feed(salt);
        h.feed(k);
        auto raw_hash = h.get_hash();
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

template<WellBehaved Key, typename Hasher = SimpleHasher> class HashSet final {

public:
    void insert(const Key &key) { map.insert(key, 1); }

    bool contains(const Key &key) const { return map.contains(key); }

    void remove(const Key &k) { map.remove(k); }

    size_t size() const { return map.size(); }

private:
    // This is inefficient.
    // It is just to get the implementation going
    // and work out the API.
    // Replace with a proper implementation later.
    HashMap<Key, int, Hasher> map;
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

template<typename T> void sort_relocatable(T *data, size_t bufsize) {
    auto ordering = [](const void *v1, const void *v2) -> int {
        auto d1 = (T *)v1;
        auto d2 = (T *)v2;
        return *d1 <=> *d2;
    };
    qsort(data, bufsize, sizeof(T), ordering);
}

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

    BytesView view() const { return BytesView{(const char *)buf, 0, bufsize}; }

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

template<typename T1, typename T2> constexpr int maxval(const T1 &a, const T2 &b) {
    return a > b ? a : b;
}

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

    static constexpr int MAX_TYPES = 10;
    static_assert(sizeof...(T) <= MAX_TYPES);
    static_assert(sizeof...(T) > 0);

public:
    Variant() noexcept {
        type_id = 0;
        new(buf) T...[0]{};
    }
    Variant(const Variant<T...> &o) { copy_value_in(o, false); }
    Variant(Variant<T...> &&o) noexcept { move_to_uninitialized_memory(o); }

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
#define PYSTD2015_VAR_MOVE_SWITCH(i)                                                               \
    {                                                                                              \
        if constexpr(i < sizeof...(T)) {                                                           \
            new(buf) T... [i] { move(o.get<T...[i]>()) };                                          \
        }                                                                                          \
    }                                                                                              \
    break;

    void move_to_uninitialized_memory(Variant<T...> &&o) noexcept {
        switch(o.type_id) {
        case 0:
            PYSTD2015_VAR_MOVE_SWITCH(0);
        case 1:
            PYSTD2015_VAR_MOVE_SWITCH(1);
        case 2:
            PYSTD2015_VAR_MOVE_SWITCH(2);
        case 3:
            PYSTD2015_VAR_MOVE_SWITCH(3);
        case 4:
            PYSTD2015_VAR_MOVE_SWITCH(4);
        case 5:
            PYSTD2015_VAR_MOVE_SWITCH(5);
        case 6:
            PYSTD2015_VAR_MOVE_SWITCH(6);
        case 7:
            PYSTD2015_VAR_MOVE_SWITCH(7);
        case 8:
            PYSTD2015_VAR_MOVE_SWITCH(8);
        case 9:
            PYSTD2015_VAR_MOVE_SWITCH(9);
        default:
            internal_failure("Unreachable code in variant move.");
        }
        type_id = o.type_id;
    }

// FIXME, update to do a constexpr check to see if the
// value can be nothrow copied.
#define PYSTD2015_VAR_COPY_SWITCH(i)                                                               \
    {                                                                                              \
        T...[i] tmp(o.get<T...[i]>());                                                             \
        if(has_existing_value) {                                                                   \
            destroy();                                                                             \
        }                                                                                          \
        new(buf) T...[i](move(tmp));                                                               \
    }                                                                                              \
    break;

    void copy_value_in(const Variant<T...> &o, bool has_existing_value) {
        // If copy construction throws, keep the old value.
        switch(o.type_id) {
        case 0:
            PYSTD2015_VAR_COPY_SWITCH(0);
        case 1:
            PYSTD2015_VAR_COPY_SWITCH(1);
        case 2:
            PYSTD2015_VAR_COPY_SWITCH(2);
        case 3:
            PYSTD2015_VAR_COPY_SWITCH(3);
        case 4:
            PYSTD2015_VAR_COPY_SWITCH(4);
        case 5:
            PYSTD2015_VAR_COPY_SWITCH(5);
        case 6:
            PYSTD2015_VAR_COPY_SWITCH(6);
        case 7:
            PYSTD2015_VAR_COPY_SWITCH(7);
        case 8:
            PYSTD2015_VAR_COPY_SWITCH(8);
        case 9:
            PYSTD2015_VAR_COPY_SWITCH(9);
        default:
            internal_failure("Unreachable code in variant copy.");
        }

        type_id = o.type_id;
    }

#define PYSTD2015_VAR_COMPARE_SWITCH(i)                                                            \
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
            PYSTD2015_VAR_COMPARE_SWITCH(0);
        case 1:
            PYSTD2015_VAR_COMPARE_SWITCH(1);
        case 2:
            PYSTD2015_VAR_COMPARE_SWITCH(2);
        case 3:
            PYSTD2015_VAR_COMPARE_SWITCH(3);
        case 4:
            PYSTD2015_VAR_COMPARE_SWITCH(4);
        case 5:
            PYSTD2015_VAR_COMPARE_SWITCH(5);
        case 6:
            PYSTD2015_VAR_COMPARE_SWITCH(6);
        case 7:
            PYSTD2015_VAR_COMPARE_SWITCH(7);
        case 8:
            PYSTD2015_VAR_COMPARE_SWITCH(8);
        case 9:
            PYSTD2015_VAR_COMPARE_SWITCH(9);
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

// Note: uses C format specifiers currently.
typedef void (*StringFormatCallback)(const char *buf, size_t bufsize, void *ctx);
CString format(const char *format, ...);
void format_with_cb(StringFormatCallback cb, void *ctx, const char *format, va_list ap);

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
    format_with_cb(converter, &oobj, format, args);
    va_end(args);
}

} // namespace pystd2025
