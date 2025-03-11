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
    explicit unique_arr(T *t) noexcept  : ptr{t}, size{0} {}
    explicit unique_arr(const unique_arr<T> &o) = delete;
    explicit unique_arr(unique_arr<T> &&o) noexcept : ptr{o.ptr}, size{o.size} { o.ptr = nullptr; o.size = 0;}
    ~unique_arr() { delete[] ptr; }

    T &operator=(const unique_arr<T> &o) = delete;

    // unique_ptr<T> &
    void operator=(unique_arr<T> &&o) noexcept {
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
    size_t size;
};

enum class EncodingPolicy {
    Enforce,
    Substitute,
    Ignore,
};

class Bytes {
public:
    Bytes() noexcept;
    Bytes(const char *buf, size_t bufsize) noexcept;
    Bytes(Bytes &&o) noexcept;
    Bytes(const Bytes &o) noexcept;
    const char *c_str() const { return str.get(); }
    const char* data() const { return str.get(); }
    char* data() { return str.get(); }

    void append(const char c);

    bool empty() const { return strsize == 0; }

    size_t size() const { return strsize; }

    void extend(size_t num_bytes) noexcept;
    void shrink(size_t num_bytes) noexcept;

private:
    void double_size();

    unique_arr<char> str; // Always zero terminated.
    size_t strsize;
    size_t capacity;
};

class U8String {
public:
    U8String(U8String &&o) noexcept : bytes{move(o.bytes)} {}
    U8String(const U8String &o) noexcept : bytes{o.bytes} {}
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

template<typename T>
class Vector final {
public:
    Vector() noexcept;
    ~Vector() {
        for(size_t i=0; i<size(); ++i) {
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
        T* obj = objptr(size()-1);
        obj->~obj();
        backing.shrink(sizeof(T));
    }

    size_t size() noexcept {
        return backing.size() / sizeof(T);
    }

    T& operator[](size_t i) {
        if(i >= size()) {
            throw "Index out of bounds.";
        }
        return *objptr(i);
    }

    const T& operator[](size_t i) const {
        if(i >= size()) {
            throw "Index out of bounds.";
        }
        return *objptr(i);
    }

private:
    T* objptr(size_t i) noexcept {
        return reinterpret_cast<T*>(rawptr(i));
    }
    const T* objptr(size_t i) const noexcept {
        return reinterpret_cast<const T*>(rawptr(i));
    }
    char* rawptr(size_t i) noexcept {
        return backing.data() + i*sizeof(T);
    }
    const T* rawptr(size_t i) const noexcept {
        return backing.data() + i*sizeof(T);
    }

    Bytes backing;
};

} // namespace pystd
