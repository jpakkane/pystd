// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

void *operator new(size_t, void *ptr) noexcept;

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

class PyException;

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

    char operator[](size_t i) const { return buf[i]; }

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

template<typename T> class Vector final {
public:
    Vector() noexcept = default;
    ~Vector() {
        for(size_t i = 0; i < num_entries; ++i) {
            objptr(i)->~T();
        }
    }

    void push_back(const T &obj) noexcept {
        backing.extend(sizeof(T));
        auto obj_loc = objptr(num_entries);
        new(obj_loc) T(obj);
        ++num_entries;
    }

    void pop_back() noexcept {
        if(num_entries == 0) {
            return;
        }
        T *obj = objptr(num_entries);
        obj->~obj();
        backing.shrink(sizeof(T));
        --num_entries;
    }

    size_t capacity() noexcept { return backing.size() / sizeof(T); }
    size_t size() noexcept { return num_entries; }

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

    const T *cbegin() const { return objptr(0); }
    const T *cend() const { return objptr(num_entries); }

    T *begin() { return objptr(0); }
    T *end() { return objptr(num_entries); }

private:
    T *objptr(size_t i) noexcept { return reinterpret_cast<T *>(rawptr(i)); }
    const T *objptr(size_t i) const noexcept { return reinterpret_cast<const T *>(rawptr(i)); }
    char *rawptr(size_t i) noexcept { return backing.data() + i * sizeof(T); }
    const T *rawptr(size_t i) const noexcept { return backing.data() + i * sizeof(T); }

    Bytes backing;
    size_t num_entries = 0;
};

// Iterates over the code points of a valid UTF 8 string.
// If the string used is not valid UTF-8, result is undefined.
class ValidU8Iterator {
public:
    friend class U8String;

    ValidU8Iterator(const ValidU8Iterator &) = default;
    ValidU8Iterator(ValidU8Iterator &&) = default;

    uint32_t operator*();

    ValidU8Iterator &operator++();
    ValidU8Iterator operator++(int);

    bool operator==(const ValidU8Iterator &o) const;
    bool operator!=(const ValidU8Iterator &o) const;

    struct CharInfo {
        uint32_t codepoint;
        uint32_t byte_count;
    };

private:

    explicit ValidU8Iterator(const unsigned char *utf8_string)
        : buf{utf8_string}, has_char_info{false} {}

    void compute_char_info();

    const unsigned char *buf;
    CharInfo char_info;
    bool has_char_info;
};

class ValidU8ReverseIterator {
public:
    friend class U8String;

    ValidU8ReverseIterator(const ValidU8ReverseIterator &) = default;
    ValidU8ReverseIterator(ValidU8ReverseIterator &&) = default;

    uint32_t operator*();

    ValidU8ReverseIterator &operator++();
    ValidU8ReverseIterator operator++(int);

    bool operator==(const ValidU8ReverseIterator &o) const;
    bool operator!=(const ValidU8ReverseIterator &o) const;

private:

    explicit ValidU8ReverseIterator(const unsigned char *utf8_string, int64_t offset)
        : original_str{utf8_string}, offset{offset}, has_char_info{false} {
        go_backwards();
    }

    void go_backwards();
    void compute_char_info();

    const unsigned char *original_str;
    int64_t offset; // Need to represent -1;
    ValidU8Iterator::CharInfo char_info;
    bool has_char_info;
};


class U8String {
public:
    U8String(U8String &&o) noexcept = default;
    U8String(const U8String &o) noexcept = default;
    U8String(Bytes incoming);
    explicit U8String(const char *txt, size_t txtsize = -1);
    const char *c_str() const { return bytes.data(); }

    size_t size_bytes() const {
        auto s = bytes.size();
        if(s > 0) {
            --s; // Do not count null terminator.
        }
        return s;
    }

    U8String substr(size_t offset, size_t length) const;

    void operator=(U8String &&o) {
        if(this != &o) {
            bytes = move(o.bytes);
        }
    }

    bool operator==(const U8String &o) const { return bytes == o.bytes; }
    bool operator<(const U8String &o) const { return bytes < o.bytes; }
    // Fixme: add <=>

    template<typename Hasher> void feed_hash(Hasher &h) const { bytes.feed_hash(h); }

    Vector<U8String> split() const;

    ValidU8Iterator cbegin() const { return ValidU8Iterator((const unsigned char *)bytes.data()); }

    ValidU8Iterator cend() const {
        return ValidU8Iterator((const unsigned char *)bytes.data() + size_bytes());
    }

    ValidU8ReverseIterator crbegin() const { return ValidU8ReverseIterator((const unsigned char *)bytes.data(), size_bytes()); }

    ValidU8ReverseIterator crend() const {
        return ValidU8ReverseIterator((const unsigned char *)bytes.data(), -1);
    }

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

template<typename Key, typename Value, typename Hasher = SimpleHasher> class HashMap final {
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

template<typename T> void sort_relocatable(T *data, size_t bufsize) {
    auto ordering = [](const void *v1, const void *v2) -> int {
        auto d1 = (T *)v1;
        auto d2 = (T *)v2;
        return *d1 <=> *d2;
    };
    qsort(data, bufsize, sizeof(T), ordering);
}

} // namespace pystd2025
