// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>

namespace pystd {

template<class T> struct remove_reference { typedef T type; };
template<class T> struct remove_reference<T&> { typedef T type; };
template<class T> struct remove_reference<T&&> { typedef T type; };
template< class T >
using remove_reference_t = typename remove_reference<T>::type;

template< class T >
constexpr remove_reference_t<T>&& move( T&& t ) noexcept {
    return static_cast<T&&>(t);
}

enum class EncodingPolicy {
    Enforce,
    Substitute,
    Ignore,
};

class Bytes {
public:
    explicit Bytes(const char *buf, size_t bufsize);
    const char * c_str() const { return str;}

private:
    const char *str; // Always zero terminated.
    size_t strsize;
    size_t capacity;
};

class U8String {
public:
    explicit U8String(const char *txt, size_t txtsize);
    const char * c_str() const { return bytes.c_str(); }
private:
    Bytes bytes;
};

class PyException {
public:
    PyException(U8String msg) : message{msg} {}

    const U8String& what() const {
        return message;
    }

private:
    U8String message;
};

class File {
public:

    U8String readline();
    //Vector<U8String> readlines();
    //Vector<Bytes> readlines_raw();
private:
    FILE *f;
    Bytes buf;
};

}
