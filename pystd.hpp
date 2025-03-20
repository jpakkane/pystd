// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>
#include <stdint.h>

namespace pystd {

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

template<typename T> class unique_ptr final {
public:
    unique_ptr() : ptr{nullptr} {}
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

    T *get() { return ptr; }
    T &operator*() { return *ptr; }
    T *operator->() { return ptr; }

private:
    T *ptr;
};

template<typename T> class unique_arr final {
public:
    unique_arr() noexcept : ptr{nullptr} {}
    explicit unique_arr(T *t, size_t size = 0) noexcept : ptr{t}, bufsize{size} {}
    explicit unique_arr(const unique_arr<T> &o) = delete;
    explicit unique_arr(unique_arr<T> &&o) noexcept : ptr{o.ptr}, bufsize{o.bufsize} {
        o.ptr = nullptr;
        o.bufsize = 0;
    }
    ~unique_arr() { delete[] ptr; }

    T &operator=(const unique_arr<T> &o) = delete;

    // unique_ptr<T> &
    void operator=(unique_arr<T> &&o) noexcept {
        if(this != &o) {

            delete[] ptr;
            ptr = o.ptr;
            bufsize = o.bufsize;
            o.ptr = nullptr;
            o.bufsize = 0;
        }
        // return *this;
    }

    size_t size() const { return bufsize; }

    T *get() { return ptr; }
    const T *get() const { return ptr; }
    T *operator->() { return ptr; }

    T &operator[](size_t index) {
        if(index >= bufsize) {
            throw "Index out of bounds.";
        }
        return ptr[index];
    }

private:
    T *ptr;
    size_t bufsize;
};

enum class EncodingPolicy {
    Enforce,
    Substitute,
    Ignore,
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

    bool empty() const { return strsize == 0; }

    size_t size() const { return strsize; }

    void extend(size_t num_bytes) noexcept;
    void shrink(size_t num_bytes) noexcept;

    void assign(const char *buf, size_t bufsize);

private:
    void grow_to(size_t new_size);

    unique_arr<char> buf; // Not zero terminated.
    size_t strsize;
};

class U8String {
public:
    U8String(U8String &&o) noexcept : bytes{move(o.bytes)} {}
    U8String(const U8String &o) noexcept : bytes{o.bytes} {}
    explicit U8String(const char *txt, size_t txtsize = -1);
    const char *c_str() const { return bytes.data(); }

    size_t size_bytes() const { return bytes.size(); }

    U8String substr(size_t offset, size_t length) const;

private:
    Bytes bytes;
    // Store length in codepoints.
};

class PyException {
public:
    PyException(U8String msg) : message{msg} {}

    const U8String &what() const { return message; }

private:
    U8String message;
};

class File {
public:
    File(const char *fname, const char *modes);

    Bytes readline_bytes();
    ~File();

    File &operator=(File &&o) = delete;
    File &operator=(const File &o) = delete;
    // Vector<U8String> readlines();
    // Vector<Bytes> readlines_raw();

private:
    FILE *f;
    EncodingPolicy policy;
};

template<typename T> class Vector final {
public:
    Vector() noexcept;
    ~Vector() {
        for(size_t i = 0; i < size(); ++i) {
            objptr(i)->~T();
        }
    }

    void push_back(const T &obj) noexcept {
        const auto write_index = size();
        backing.extend(sizeof(T));
        new(rawtpr(write_index)) T(obj);
    }

    void pop_back() noexcept {
        if(backing.empty()) {
            return;
        }
        T *obj = objptr(size() - 1);
        obj->~obj();
        backing.shrink(sizeof(T));
    }

    size_t size() noexcept { return backing.size() / sizeof(T); }

    T &operator[](size_t i) {
        if(i >= size()) {
            throw "Index out of bounds.";
        }
        return *objptr(i);
    }

    const T &operator[](size_t i) const {
        if(i >= size()) {
            throw "Index out of bounds.";
        }
        return *objptr(i);
    }

private:
    T *objptr(size_t i) noexcept { return reinterpret_cast<T *>(rawptr(i)); }
    const T *objptr(size_t i) const noexcept { return reinterpret_cast<const T *>(rawptr(i)); }
    char *rawptr(size_t i) noexcept { return backing.data() + i * sizeof(T); }
    const T *rawptr(size_t i) const noexcept { return backing.data() + i * sizeof(T); }

    Bytes backing;
};

class Regex {
public:
    explicit Regex(const U8String &pattern);
    ~Regex();

    Regex() = delete;
    Regex(const Regex &o) = delete;
    Regex &operator=(const Regex &o) = delete;

    void *h() const { return handle; }

private:
    void *handle;
};

class Match {
public:
    Match(void *h, const U8String *orig) : handle{h}, original{orig} {};
    ~Match();

    Match() = delete;
    Match(Match &&o) : handle(o.handle), original{o.original} {
        o.handle = nullptr;
        o.original = nullptr;
    }
    Match &operator=(const Match &o) = delete;

    void *h() const { return handle; }

    U8String get_submatch(size_t i);

    size_t num_matches() const;

private:
    void *handle;
    const U8String *original; // Not very safe...
};

Match regex_match(const Regex &pattern, const U8String &text);

} // namespace pystd
