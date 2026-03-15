// SPDX-License-Identifier: Apache-2.0
// Copyright 2022-2026 Jussi Pakkanen

#include <pystd2026.hpp>

namespace pystd2026 {

template<typename Key, typename Value> class HashMapIterator;

template<WellBehaved Key, WellBehaved Value, WellBehaved HashAlgo = SimpleHash>
class HashMap final {
public:
    static_assert(!pystd2026::is_floating_point_v<pystd2026::remove_cv_t<Key>>,
                  "Floats can not be used as map keys as that is highly unreliable.");

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

    static_assert(!pystd2026::is_floating_point_v<pystd2026::remove_cv_t<Key>>,
                  "Floats can not be used as set keys as that is highly unreliable.");

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

} // namespace pystd2026
