// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>
#include <stdint.h>

void* operator new(size_t, void *ptr) noexcept;


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
    explicit unique_arr(size_t size) noexcept : ptr{new T[size]}, bufsize{size} {}
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

    const T &operator[](size_t index) const {
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

class SimpleHasher final {
public:
    void feed_bytes(const char *buf, size_t bufsize) noexcept;

    template<typename T> void feed_primitive(const T &c) noexcept {
        // FIXME, add concept to only accept ints and floats.
        feed_bytes((const char *)&c, sizeof(c));
    }

    size_t get_hash() const { return value; }

private:
    size_t value{0};
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

    void operator=(Bytes &&o) {
        if(this != &o) {
            buf = move(o.buf);
            strsize = o.strsize;
            o.strsize = 0;
        }
    }

    bool operator==(const Bytes &o) const {
        if(strsize != o.strsize) {
            return false;
        }
        for(size_t i = 0; i < strsize; ++i) {
            if(buf[i] != o.buf[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator<(const Bytes &o) const {
        const auto num_its = strsize < o.strsize ? strsize : o.strsize;
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
        return strsize < o.strsize;
    }

    template<typename Hasher> void feed_hash(Hasher &h) const { h.feed_bytes(buf.get(), strsize); }

private:
    void grow_to(size_t new_size);

    unique_arr<char> buf; // Not zero terminated.
    size_t strsize;
};

class U8String {
public:
    U8String(U8String &&o) noexcept = default;
    U8String(const U8String &o) noexcept = default;
    explicit U8String(const char *txt, size_t txtsize = -1);
    const char *c_str() const { return bytes.data(); }

    size_t size_bytes() const { return bytes.size(); }

    U8String substr(size_t offset, size_t length) const;

    void operator=(U8String &&o) {
        if(this != &o) {
            bytes = move(o.bytes);
        }
    }

    bool operator==(const U8String &o) const = default;
    bool operator<(const U8String &o) const { return bytes < o.bytes; }

    template<typename Hasher> void feed_hash(Hasher &h) const { bytes.feed_hash(h); }

private:
    Bytes bytes;
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

template<typename Key, typename Value, typename Hasher = SimpleHasher> class HashMap final {
public:
    HashMap() {
        salt = (size_t)this;
        num_entries = 0;
        size_in_powers_of_two = 6;
        auto initial_table_size = 1 << size_in_powers_of_two;
        mod_mask = initial_table_size - 1;
        data.hashes = unique_arr<size_t>(initial_table_size);
        data.reset_hash_values();
        data.keydata = Bytes(initial_table_size * sizeof(Key));
        data.valuedata = Bytes(initial_table_size * sizeof(Value));
    }

    const Value *lookup(const Key &key) const {
        const auto hashval = hash_for(key);
        auto slot = hashval & mod_mask;
        while(true) {
            if(data.hashes[slot] == FREE_SLOT) {
                return nullptr;
            }
            if(data.hashes[slot] == hashval) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    return data.valueptr(slot);
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

    bool contains(const Key &key) const {
        return lookup(key) != nullptr;
    }

    size_t size() const { return num_entries; }

private:
    // This is neither fast, memory efficient nor elegant.
    // At this point in the project life cycle it does not need to be.
    struct MapData {
        unique_arr<size_t> hashes;
        Bytes keydata;
        Bytes valuedata;

        MapData() = default;
        MapData(MapData &&o) noexcept{
            hashes = move(o.hashes);
            keydata = move(o.valuedata);
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

        Key *keyptr(size_t i) noexcept { return (Key*)(keydata.data() + i * sizeof(Key)); }
        const Key *keyptr(size_t i) const noexcept { return (const Key*)(keydata.data() + i * sizeof(Key)); }

        Value *valueptr(size_t i) noexcept { return (Value*)(valuedata.data() + i * sizeof(Value)); }
        const Value *valueptr(size_t i) const noexcept {
            return (const Value*)(valuedata.data() + i * sizeof(Value));
        }

        void reset_hash_values() {
            for(size_t i = 0; i < hashes.size(); ++i) {
                hashes[i] = FREE_SLOT;
            }
        }

        void deallocate_contents() {
            for(size_t i = 0; i < hashes.size(); ++i) {
                if(hashes[i] == FREE_SLOT || hashes[i] == TOMBSTONE) {
                    continue;
                }
                keyptr(i)->~Key();
                valueptr(i)->~Value();
            }
        }
    };

    void insert_internal(size_t hashval, const Key &key, const Value &v) {
        auto slot = hashval & mod_mask;
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
        const auto new_size = 2*table_size();
        const auto new_powers_of_two = size_in_powers_of_two + 1;
        const auto new_mod_mask = new_powers_of_two - 1;
        MapData grown;

        grown.hashes = unique_arr<size_t>(new_size);
            grown.keydata = Bytes(new_size*sizeof(Key));
        grown.valuedata = Bytes(new_size*sizeof(Value));

        grown.reset_hash_values();
        MapData old = move(data);
        data = move(grown);
        size_in_powers_of_two = new_powers_of_two;
        mod_mask = new_mod_mask;
        for(size_t i=0; i<old.hashes.size(); ++i) {
            if(old.hashes[i] == FREE_SLOT || old.hashes[i] == TOMBSTONE) {
                continue;
            }
            insert_internal(old.hashes[i], *old.keyptr(i), *old.valueptr(i));
        }
    }

    size_t hash_for(const Key &k) const {
        Hasher h;
        h.feed_primitive(salt);
        k.feed_hash(h);
        auto raw_hash = h.get_hash();
        if(raw_hash == FREE_SLOT) {
            raw_hash = FREE_SLOT + 1;
        } else if(raw_hash == TOMBSTONE) {
            raw_hash = TOMBSTONE + 1;
        }
        return raw_hash;
    }

    size_t table_size() const { return data.hashes.size(); }

    double fill_ratio() const {
        return double(num_entries) / table_size();
    }
    static constexpr size_t FREE_SLOT = 42;
    static constexpr size_t TOMBSTONE = 666;
    static constexpr double MAX_LOAD = 0.7;

    MapData data;
    size_t salt;
    size_t num_entries;
    size_t mod_mask;
    int32_t size_in_powers_of_two;
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
