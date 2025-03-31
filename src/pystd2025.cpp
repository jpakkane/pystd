// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void *operator new(size_t count) { return malloc(count); }

void operator delete(void *ptr) noexcept { free(ptr); }

void *operator new(size_t, void *ptr) noexcept { return ptr; }

namespace pystd2025 {

namespace {

bool is_ascii_whitespace(char c) { return c == ' ' || c == '\n' || c == '\n' || c == '\t'; }

struct UtfDecodeStep {
    uint32_t byte1_data_mask;
    uint32_t num_subsequent_bytes;
};

const size_t default_bufsize = 16;

bool is_valid_uf8_character(const char *input,
                            size_t input_size,
                            size_t cur,
                            const UtfDecodeStep &par) {
    const uint32_t byte1 = uint32_t((unsigned char)input[cur]);
    const uint32_t subsequent_header_mask = 0b011000000;
    const uint32_t subsequent_header_value = 0b10000000;
    const uint32_t subsequent_data_mask = 0b111111;
    const uint32_t subsequent_num_data_bits = 6;

    if(cur + par.num_subsequent_bytes >= input_size) {
        return false;
    }
    uint32_t unpacked = byte1 & par.byte1_data_mask;
    for(uint32_t i = 0; i < par.num_subsequent_bytes; ++i) {
        unpacked <<= subsequent_num_data_bits;
        const uint32_t subsequent = uint32_t((unsigned char)input[cur + 1 + i]);
        if((subsequent & subsequent_header_mask) != subsequent_header_value) {
            return false;
        }
        assert((unpacked & subsequent_data_mask) == 0);
        unpacked |= subsequent & subsequent_data_mask;
    }

    return true;
}

bool is_valid_utf8(const char *input, size_t input_size) {
    if(input_size == (size_t)-1) {
        input_size = strlen(input);
    }
    UtfDecodeStep par;
    // clang-format off
    const uint32_t twobyte_header_mask    = 0b11100000;
    const uint32_t twobyte_header_value   = 0b11000000;
    const uint32_t threebyte_header_mask  = 0b11110000;
    const uint32_t threebyte_header_value = 0b11100000;
    const uint32_t fourbyte_header_mask   = 0b11111000;
    const uint32_t fourbyte_header_value  = 0b11110000;
    // clang-format on
    for(size_t i = 0; i < input_size; ++i) {
        const uint32_t code = uint32_t((unsigned char)input[i]);
        if(code < 0x80) {
            continue;
        } else if((code & twobyte_header_mask) == twobyte_header_value) {
            par.byte1_data_mask = 0b11111;
            par.num_subsequent_bytes = 1;
        } else if((code & threebyte_header_mask) == threebyte_header_value) {
            par.byte1_data_mask = 0b1111;
            par.num_subsequent_bytes = 2;
        } else if((code & fourbyte_header_mask) == fourbyte_header_value) {
            par.byte1_data_mask = 0b111;
            par.num_subsequent_bytes = 3;
        } else {
            return false;
        }
        if(!is_valid_uf8_character(input, input_size, i, par)) {
            return false;
        }
        i += par.num_subsequent_bytes;
    }
    return true;
}

uint32_t unpack_one(const unsigned char *valid_utf8, const UtfDecodeStep &par) {
    const uint32_t byte1 = uint32_t(*valid_utf8);
    const uint32_t subsequent_data_mask = 0b111111;
    const uint32_t subsequent_num_data_bits = 6;

    uint32_t unpacked = byte1 & par.byte1_data_mask;
    for(uint32_t i = 0; i < par.num_subsequent_bytes; ++i) {
        unpacked <<= subsequent_num_data_bits;
        const uint32_t subsequent = uint32_t((unsigned char)valid_utf8[1 + i]);
        assert((unpacked & subsequent_data_mask) == 0);
        unpacked |= subsequent & subsequent_data_mask;
    }

    return unpacked;
}

} // namespace

void SimpleHasher::feed_bytes(const char *buf, size_t bufsize) noexcept {
    for(size_t i = 0; i < bufsize; ++i) {
        value = 13 * value ^ (unsigned char)buf[i];
    }
}

Bytes::Bytes() noexcept : buf{new char[default_bufsize], default_bufsize} { strsize = 0; }

Bytes::Bytes(size_t initial_size) noexcept : buf{new char[initial_size], initial_size} {
    strsize = 0;
}

Bytes::Bytes(const char *data, size_t bufsize) noexcept : buf{bufsize} {
    for(size_t i = 0; i < bufsize; ++i) {
        buf[i] = data[i];
    }
}

Bytes::Bytes(Bytes &&o) noexcept {
    buf = move(o.buf);
    strsize = o.strsize;
    o.strsize = 0;
}

Bytes::Bytes(const Bytes &o) noexcept {
    buf = unique_arr<char>(new char[o.buf.size()], o.buf.size());
    memcpy(buf.get(), o.buf.get(), o.buf.size());
    strsize = o.strsize;
}

void Bytes::assign(const char *buf_in, size_t bufsize) {
    strsize = 0;
    grow_to(bufsize + 1);               // Prepare for the eventual null terminator.
    memcpy(buf.get(), buf_in, bufsize); // str might not be null terminated.
    strsize = bufsize;
}

void Bytes::grow_to(size_t new_size) {
    assert(new_size < (size_t{1} << 48));
    if(buf.size() >= new_size) {
        return;
    }
    size_t new_capacity = default_bufsize;
    while(new_capacity < new_size) {
        new_capacity *= 2;
    }
    unique_arr<char> new_buf(new_capacity);
    memcpy(new_buf.get(), buf.get(), strsize);
    buf = move(new_buf);
}

void Bytes::append(const char c) {
    if(strsize >= buf.size()) {
        grow_to(buf.size() * 2);
    }
    buf[strsize] = c;
    ++strsize;
}

void Bytes::extend(size_t num_bytes) noexcept {
    const size_t target_size = strsize + num_bytes;
    grow_to(target_size);
    strsize += num_bytes;
}

void Bytes::shrink(size_t num_bytes) noexcept {
    if(num_bytes > strsize) {
        strsize = 0;
    } else {
        strsize -= num_bytes;
    }
}

uint32_t ValidU8Iterator::operator*() {
    compute_char_info();
    return char_info.codepoint;
}

ValidU8Iterator &ValidU8Iterator::operator++() {
    compute_char_info();
    buf += char_info.byte_count;
    has_char_info = false;
    return *this;
}

ValidU8Iterator ValidU8Iterator::operator++(int) {
    compute_char_info();
    ValidU8Iterator rval{*this};
    ++(*this);
    return rval;
}

bool ValidU8Iterator::operator==(const ValidU8Iterator &o) const { return buf == o.buf; }

bool ValidU8Iterator::operator!=(const ValidU8Iterator &o) const { return !(*this == o); }

ValidU8Iterator::CharInfo ValidU8Iterator::extract_one_codepoint(const unsigned char *buf) {
    UtfDecodeStep par;
    // clang-format off
    const uint32_t twobyte_header_mask    = 0b11100000;
    const uint32_t twobyte_header_value   = 0b11000000;
    const uint32_t threebyte_header_mask  = 0b11110000;
    const uint32_t threebyte_header_value = 0b11100000;
    const uint32_t fourbyte_header_mask   = 0b11111000;
    const uint32_t fourbyte_header_value  = 0b11110000;
    // clang-format on
    const uint32_t code = uint32_t((unsigned char)buf[0]);
    if(code < 0x80) {
        return CharInfo{code, 1};
    } else if((code & twobyte_header_mask) == twobyte_header_value) {
        par.byte1_data_mask = 0b11111;
        par.num_subsequent_bytes = 1;
    } else if((code & threebyte_header_mask) == threebyte_header_value) {
        par.byte1_data_mask = 0b1111;
        par.num_subsequent_bytes = 2;
    } else if((code & fourbyte_header_mask) == fourbyte_header_value) {
        par.byte1_data_mask = 0b111;
        par.num_subsequent_bytes = 3;
    } else {
        abort();
    }
    return CharInfo{unpack_one(buf, par), 1 + par.num_subsequent_bytes};
}

U8String::U8String(Bytes incoming) {
    if(!is_valid_utf8(incoming.data(), incoming.size())) {
        throw PyException("Invalid UTF-8.");
    }
    bytes = move(incoming);
    bytes.append('\0');
}

U8String::U8String(const char *txt, size_t txtsize) {
    if(txtsize == (size_t)-1) {
        txtsize = strlen(txt);
    }
    if(!is_valid_utf8(txt, txtsize)) {
        throw PyException(U8String{"Invalid UTF-8."});
    }
    bytes.assign(txt, txtsize);
    bytes.append('\0');
}

U8String U8String::substr(size_t offset, size_t length) const {
    // FIXME, validate range.
    return U8String(bytes.data() + offset, length);
}

Vector<U8String> U8String::split() const {
    Vector<U8String> arr;
    size_t i = 0;
    while(i < size_bytes()) {
        while(i < size_bytes() && is_ascii_whitespace(bytes[i])) {
            ++i;
        }
        if(i == size_bytes()) {
            break;
        }
        const auto string_start = i;
        ++i;
        while(i < size_bytes() && (!is_ascii_whitespace(bytes[i]))) {
            ++i;
        }
        auto sub = substr(string_start, i - string_start);
        arr.push_back(sub);
    }
    return arr;
}

void ValidU8Iterator::compute_char_info() {
    if(has_char_info) {
        return;
    }
    char_info = extract_one_codepoint(buf);
};

PyException::PyException(const char *msg) : message("") {
    Bytes b;
    size_t offset = 0;
    while(msg[offset]) {
        auto c = msg[offset++];
        if(c > 127) {
            b.append('?');
        } else {
            b.append(c);
        }
    }
    message = U8String(b.data(), b.size());
}

bool FileLineIterator::operator!=(const FileEndSentinel &) const { return !f->eof(); }

Bytes &&FileLineIterator::operator*() {
    if(!is_up_to_date) {
        line = f->readline_bytes();
    }
    return move(line);
}

File::File(const char *fname, const char *modes) : policy{EncodingPolicy::Enforce} {
    f = fopen(fname, modes);
    if(!f) {
        abort();
    }
}

File::~File() { fclose(f); }

Bytes File::readline_bytes() {
    Bytes b(160);
    // The world's least efficient readline.
    char c;
    while(true) {
        auto rc = fread(&c, 1, 1, f);
        if(rc != 1) {
            if(feof(f)) {
                if(!b.empty()) {
                    b.append('\0');
                }
                return b;
            } else if(ferror(f)) {
                abort();
            } else {
                abort();
            }
        }
        b.append(c);
        if(c == '\n') {
            return b;
        }
    }
}

} // namespace pystd2025
