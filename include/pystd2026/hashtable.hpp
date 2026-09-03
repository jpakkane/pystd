// SPDX-License-Identifier: Apache-2.0
// Copyright 2022-2026 Jussi Pakkanen

#include <pystd2026/common.hpp>

namespace pystd2026 {

template<typename Key, typename Value> class HashTableCommonIterator;

struct SetOnlyTag {};

template<WellBehaved Key, WellBehaved Value, WellBehaved HashAlgo = SimpleHash>
class HashTableCommon {
private:
    static constexpr size_t key_val_padding =
        alignof(Key) >= alignof(Value) ? 0 : alignof(Value) - alignof(Key);
    static_assert(key_val_padding >= 0);

protected:
    static_assert(!::pystd2026::is_floating_point_v<::pystd2026::remove_cv_t<Key>>,
                  "Floats can not be used as map keys as that is highly unreliable.");
    static_assert(!::pystd2026::is_reference_v<Key>);
    static_assert(!::pystd2026::is_reference_v<Value>);

    static constexpr bool set_only = ::pystd2026::is_same_v<Value, SetOnlyTag>;

    friend class HashTableCommonIterator<Key, Value>;
    HashTableCommon() noexcept {
        salt = reinterpret_cast<size_t>(this);
        num_entries = 0;
        size_in_powers_of_two = MIN_SIZE_POWER_OF_TWO;
        auto initial_table_size = 1 << size_in_powers_of_two;
        data.md = unique_arr<SlotMetadata>(initial_table_size);
        data.reset_hash_values();
        if constexpr(set_only) {
            data.data_buffer = Bytes(initial_table_size * sizeof(Key));
        } else {
            data.data_buffer = Bytes(initial_table_size * sizeof(Key) + key_val_padding +
                                     initial_table_size * sizeof(Value));
        }
    }

    ::pystd2026::Optional<size_t> lookup_slot(const Key &key) const {
        const auto hashval = hash_for(key);
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.md[slot].state == SlotState::Empty) {
                return {};
            } else if(data.md[slot].state == SlotState::Tombstone) {

            } else if(data.md[slot].bloom_matches(hashval)) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    return slot;
                }
            }
            slot = (slot + 1) & mod_mask();
        }
    }

    Value *lookup(const Key &key) const {
        auto slot = lookup_slot(key);
        if(slot) {
            return const_cast<Value *>(data.valueptr(*slot));
        } else {
            return nullptr;
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
        const auto hashval = hash_for(key);
        auto result_slot = insert_internal(hashval, key, ::pystd2026::move(v));
        return *const_cast<Value *>(data.valueptr(result_slot));
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
                    const auto previous_slot = (slot + table_size() - 1) & mod_mask();
                    const auto next_slot = (slot + 1) & mod_mask();
                    if(data.md[previous_slot].state == SlotState::Empty &&
                       data.md[next_slot].state == SlotState::Empty) {
                        data.md[slot].state = SlotState::Empty;
                    } else {
                        data.md[slot].state = SlotState::Tombstone;
                    }
                    --num_entries;
                    if(fill_percentage() < MIN_LOAD_PERCENTAGE) {
                        shrink();
                    }
                    return;
                }
            }
            slot = (slot + 1) & mod_mask();
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

    HashTableCommonIterator<Key, Value> begin() const {
        return HashMapIterator<Key, Value>(const_cast<HashTableCommon *>(this), 0);
    }

    HashTableCommonIterator<Key, Value> end() const {
        return HashMapIterator<Key, Value>(const_cast<HashTableCommon *>(this), table_size());
    }

    void clear() {
        data.clear();
        num_entries = 0;
    }

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
        // Keys first, then values.
        Bytes data_buffer;

        MapData() noexcept = default;
        MapData(MapData &&o) noexcept {
            md = move(o.md);
            data_buffer = move(o.data_buffer);
        }

        void operator=(MapData &&o) noexcept {
            if(this != &o) {
                md = move(o.md);
                data_buffer = move(o.data_buffer);
            }
        }

        ~MapData() { deallocate_contents(); }

        Key *keyptr(size_t i) noexcept { return (Key *)(data_buffer.data() + i * sizeof(Key)); }
        const Key *keyptr(size_t i) const noexcept {
            return (const Key *)(data_buffer.data() + i * sizeof(Key));
        }

        Value *valueptr(size_t i) noexcept {
            return (Value *)(data_buffer.data() + md.size() * sizeof(Key) + key_val_padding +
                             i * sizeof(Value));
        }
        const Value *valueptr(size_t i) const noexcept {
            return (const Value *)(data_buffer.data() + md.size() * sizeof(Key) + key_val_padding +
                                   i * sizeof(Value));
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

        bool has_value_at(size_t offset) const noexcept {
            if(offset >= md.size()) {
                return false;
            }
            return md[offset].state == SlotState::HasValue;
        }
    };

    size_t hash_to_slot(size_t hashval) const noexcept {
        const size_t total_bits = sizeof(size_t) * 8;
        size_t consumed_bits = 0;
        size_t slot = 0;
        while(consumed_bits < total_bits) {
            slot ^= hashval & mod_mask();
            consumed_bits += size_in_powers_of_two;
            hashval >>= size_in_powers_of_two;
        }
        return slot;
    }

    size_t insert_internal(const Key &key, Value &&v) noexcept {
        auto hashval = hash_for(key);
        if(fill_percentage() >= MAX_LOAD_PERCENTAGE) {
            grow();
        }
        auto slot = hash_to_slot(hashval);
        while(true) {
            if(data.md[slot].state != SlotState::HasValue) {
                auto *key_loc = data.keyptr(slot);
                auto *value_loc = data.valueptr(slot);
                new(key_loc) Key(key);
                if constexpr(!set_only) {
                    new(value_loc) Value{::pystd2026::move(v)};
                }
                data.md[slot] = SlotMetadata{SlotState::HasValue, (uint8_t)(hashval & BLOOM_MASK)};
                ++num_entries;
                return slot;
            }
            if(data.md[slot].bloom_matches(hashval)) {
                auto *potential_key = data.keyptr(slot);
                if(*potential_key == key) {
                    if constexpr(!set_only) {
                        auto *value_loc = data.valueptr(slot);
                        *value_loc = ::pystd2026::move(v);
                    }
                    return slot;
                }
            }
            slot = (slot + 1) & mod_mask();
        }
    }

    void grow() {
        const auto new_powers_of_two = size_in_powers_of_two + 1;
        const auto new_table_size = 2 * table_size();
        resize_to(new_powers_of_two, new_table_size);
    }

    void shrink() {
        if(table_size() >= MIN_SIZE) {
            const auto new_powers_of_two = size_in_powers_of_two - 1;
            const auto new_table_size = table_size() / 2;
            resize_to(new_powers_of_two, new_table_size);
        }
    }

    void resize_to(uint32_t new_powers_of_two, size_t new_size) {
        MapData resized;

        resized.md = unique_arr<SlotMetadata>(new_size);
        if(set_only) {
            resized.data_buffer = Bytes(new_size * sizeof(Key));
        } else {
            resized.data_buffer =
                Bytes(new_size * sizeof(Key) + key_val_padding + new_size * sizeof(Value));
        }

        resized.reset_hash_values();
        MapData old = ::pystd2026::move(data);
        data = ::pystd2026::move(resized);
        size_in_powers_of_two = new_powers_of_two;
        num_entries = 0;
        for(size_t i = 0; i < old.md.size(); ++i) {
            if(old.md[i].state == SlotState::HasValue) {
                auto &old_key = *old.keyptr(i);
                if constexpr(!set_only) {
                    insert_internal(old_key, ::pystd2026::move(*old.valueptr(i)));
                } else {
                    insert_internal(old_key, SetOnlyTag{});
                }
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

    size_t mod_mask() const { return (size_t{1} << size_in_powers_of_two) - 1; }

    size_t table_size() const { return data.md.size(); }

    size_t fill_percentage() const { return (num_entries * 100) / table_size(); }

    static constexpr size_t MAX_LOAD_PERCENTAGE = 77;
    static constexpr size_t MIN_LOAD_PERCENTAGE = 25;
    static const size_t MIN_SIZE = 16;
    static const size_t MIN_SIZE_POWER_OF_TWO = 4;

    MapData data;
    size_t salt;
    size_t num_entries;
    uint32_t size_in_powers_of_two;
};

template<typename Key, typename Value> struct KeyValue {
    Key *key;
    Value *value;
};

template<typename Key, typename Value> class HashTableCommonIterator final {
public:
    HashTableCommonIterator(HashTableCommon<Key, Value> *table, size_t offset)
        : table{table}, offset{offset} {
        if(offset == 0 && !table->data.has_value_at(offset)) {
            advance();
        }
    }

    KeyValue<Key, Value> operator*() {
        return KeyValue{table->data.keyptr(offset), table->data.valueptr(offset)};
    }

    bool operator!=(const HashTableCommonIterator<Key, Value> &o) const {
        return offset != o.offset;
    };

    HashTableCommonIterator<Key, Value> &operator++() {
        advance();
        return *this;
    }

private:
    void advance() {
        ++offset;
        while(offset < table->table_size() && !table->data.has_value_at(offset)) {
            ++offset;
        }
    }

    HashTableCommon<Key, Value> *table;
    size_t offset;
};

template<WellBehaved Key, WellBehaved Value, WellBehaved HashAlgo = SimpleHash>
class HashMap final : private HashTableCommon<Key, Value, HashAlgo> {
public:
    HashMap() noexcept = default;

    Value *lookup(const Key &key) const {
        auto slot = lookup_slot(key);
        if(slot) {
            return const_cast<Value *>(data.valueptr(*slot));
        } else {
            return nullptr;
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
        auto result_slot = insert_internal(key, ::pystd2026::move(v));
        return *const_cast<Value *>(data.valueptr(result_slot));
    }

    Value &operator[](const Key &k) {
        auto *c = lookup(k);
        if(c) {
            return (*c);
        } else {
            return insert(k, Value{});
        }
    }

    using HashTableCommon<Key, Value, HashAlgo>::remove;

    bool contains(const Key &key) const { return lookup(key) != nullptr; }

    using HashTableCommon<Key, Value, HashAlgo>::size;

    bool is_empty() const { return size() == 0; }

    HashTableCommonIterator<Key, Value> begin() const {
        return HashTableCommonIterator<Key, Value>(const_cast<HashMap *>(this), 0);
    }

    HashTableCommonIterator<Key, Value> end() const {
        return HashTableCommonIterator<Key, Value>(const_cast<HashMap *>(this), table_size());
    }

    using HashTableCommon<Key, Value, HashAlgo>::clear;
    using HashTableCommon<Key, Value, HashAlgo>::table_size;

private:
    using HashTableCommon<Key, Value, HashAlgo>::data;
    using HashTableCommon<Key, Value, HashAlgo>::lookup_slot;
    using HashTableCommon<Key, Value, HashAlgo>::hash_for;
    using HashTableCommon<Key, Value, HashAlgo>::insert_internal;
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

template<WellBehaved Key, WellBehaved HashAlgo = SimpleHash>
class HashSet final : private HashTableCommon<Key, SetOnlyTag, HashAlgo> {

public:
    HashSet() noexcept = default;

    HashInsertResult insert(const Key &key) {
        // FIXME, inefficient. Does the lookup twice.
        bool inserted = !contains(key);
        if(inserted) {
            insert_internal(key, {});
        }
        return HashInsertResult{42, inserted};
    }

    bool contains(const Key &key) const { return lookup_slot(key); }

    using HashTableCommon<Key, SetOnlyTag, HashAlgo>::remove;

    using HashTableCommon<Key, SetOnlyTag, HashAlgo>::size;

    bool is_empty() const noexcept { return size() == 0; }

    using HashTableCommon<Key, SetOnlyTag, HashAlgo>::clear;

    /*
    HashSetIterator<Key> begin() const { return HashSetIterator<Key>(map.begin()); }

    HashSetIterator<Key> end() const { return HashSetIterator<Key>(map.end()); }
    */

private:
    using HashTableCommon<Key, SetOnlyTag, HashAlgo>::insert_internal;
    using HashTableCommon<Key, SetOnlyTag, HashAlgo>::lookup_slot;
};

} // namespace pystd2026
