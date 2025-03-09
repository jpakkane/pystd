// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>

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
    explicit unique_ptr(T *t) : ptr{t} {}
    explicit unique_ptr(const unique_ptr<T> &o) = delete;
    explicit unique_ptr(unique_ptr<T> &&o) : ptr{o.ptr} { o.ptr = nullptr; }
    ~unique_ptr() { delete ptr; }

    T &operator=(const unique_ptr<T> &o) = delete;

    T &operator=(unique_ptr<T> &&o) {
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
    unique_arr() : ptr{nullptr} {}
    explicit unique_arr(T *t) : ptr{t} {}
    explicit unique_arr(const unique_arr<T> &o) = delete;
    explicit unique_arr(unique_arr<T> &&o) : ptr{o.ptr} { o.ptr = nullptr; }
    ~unique_arr() { delete[] ptr; }

    T &operator=(const unique_arr<T> &o) = delete;

    // unique_ptr<T> &
    void operator=(unique_arr<T> &&o) {
        if(this != &o) {
            delete ptr;
            ptr = o.ptr;
            o.ptr = nullptr;
        }
        // return *this;
    }

    T *get() { return ptr; }
    const T *get() const { return ptr; }
    T *operator->() { return ptr; }

    T &operator[](size_t index) {
        // FIXME, add bounds checks.
        return ptr[index];
    }

private:
    T *ptr;
};

enum class EncodingPolicy {
    Enforce,
    Substitute,
    Ignore,
};

class Bytes {
public:
    Bytes();
    Bytes(const char *buf, size_t bufsize);
    Bytes(Bytes &&o);
    Bytes(const Bytes &o);
    const char *c_str() const { return str.get(); }

    void append(const char c);

    bool empty() const { return strsize == 0; }

private:
    void double_size();

    unique_arr<char> str; // Always zero terminated.
    size_t strsize;
    size_t capacity;
};

class U8String {
public:
    U8String(U8String &&o) : bytes{move(o.bytes)} {}
    U8String(const U8String &o) : bytes{o.bytes} {}
    explicit U8String(const char *txt, size_t txtsize);
    const char *c_str() const { return bytes.c_str(); }

private:
    Bytes bytes;
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

} // namespace pystd
