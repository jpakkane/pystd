// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <pystd2025_tables.hpp>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

/*
 * These two are needed when building a shared library but not
 * with a static one. For now support only static.
 */
/*
void *operator new(size_t count) { return malloc(count); }

void operator delete(void *ptr) noexcept { free(ptr); }
void *operator new(size_t, void *ptr) noexcept { return ptr; }
*/

#ifndef __GLIBCXX__
void *operator new(size_t, void *ptr) noexcept { return ptr; }
#endif

namespace pystd2025 {

void bootstrap_throw(const char *msg) { throw PyException(msg); }

namespace {

bool is_ascii_whitespace(uint32_t c) {
    return c == ' ' || c == '\n' || c == '\n' || c == '\t' || c == '\r';
}

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

ValidatedU8Iterator::CharInfo extract_one_codepoint(const unsigned char *buf) {
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
        return ValidatedU8Iterator::CharInfo{code, 1};
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
    return ValidatedU8Iterator::CharInfo{unpack_one(buf, par), 1 + par.num_subsequent_bytes};
}

} // namespace

void internal_failure(const char *message) noexcept {
    fprintf(stderr, "Pystd encountered an internal failure: %s\n", message);
    fprintf(stderr, "Killing process for your security.\n");
    abort();
}

void SimpleHash::feed_bytes(const char *buf, size_t bufsize) noexcept {
    for(size_t i = 0; i < bufsize; ++i) {
        value = 13 * value ^ (unsigned char)buf[i];
    }
}

BytesView::BytesView(const Bytes &b) noexcept : BytesView(b.data(), b.size()) {}

Bytes::Bytes() noexcept : buf{} { bufsize = 0; }

Bytes::Bytes(size_t initial_size) noexcept : buf{new char[initial_size], initial_size} {
    bufsize = 0;
}

Bytes::Bytes(const char *data, size_t datasize) noexcept : buf{datasize} {
    memcpy(buf.get(), data, datasize);
    bufsize = datasize;
}

Bytes::Bytes(const char *buf_start, const char *buf_end) {
    if(buf_start > buf_end) {
        throw PyException("Bad range to Bytes().");
    }
    bufsize = buf_end - buf_start;
    buf = unique_arr<char>(bufsize);
    memcpy(buf.get(), buf_start, bufsize);
}

Bytes::Bytes(size_t count, char fill_value) : buf{count} {
    for(size_t i = 0; i < count; ++i) {
        buf[i] = fill_value;
    }
    bufsize = count;
}

Bytes::Bytes(Bytes &&o) noexcept {
    buf = move(o.buf);
    bufsize = o.bufsize;
    o.bufsize = 0;
}

Bytes::Bytes(const Bytes &o) noexcept
    : buf(unique_arr<char>(new char[o.buf.size()], o.buf.size())) {
    memcpy(buf.get(), o.buf.get(), o.buf.size());
    bufsize = o.bufsize;
}

void Bytes::assign(const char *buf_in, size_t in_size) {
    bufsize = 0;
    grow_to(in_size + 1);               // Prepare for the eventual null terminator.
    memcpy(buf.get(), buf_in, in_size); // str might not be null terminated.
    bufsize = in_size;
}

void Bytes::insert(size_t i, const char *buf_in, size_t in_size) {
    if(i >= bufsize) {
        throw PyException("Invalid index for insert.");
    }
    // FIXME
    assert(!is_ptr_within(buf_in));
    auto new_size = bufsize + in_size;
    grow_to(new_size);
    auto splice_point = buf.get() + i;
    auto tail_size = bufsize - i;
    auto new_tail_point = buf.get() + i + in_size;

    memmove(new_tail_point, splice_point, tail_size);
    memcpy(splice_point, buf_in, in_size);
    bufsize = new_size;
}

void Bytes::pop_back(size_t num) {
    if(num >= bufsize) {
        clear();
        return;
    }
    bufsize -= num;
}

void Bytes::pop_front(size_t num) {
    if(num >= bufsize) {
        clear();
    }
    memmove(buf.get(), buf.get() + num, buf.size() - num);
    bufsize -= num;
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
    if(bufsize > 0) {
        memcpy(new_buf.get(), buf.get(), bufsize);
    }
    buf = move(new_buf);
}

void Bytes::append(const char c) {
    if((bufsize + 1) >= buf.size()) {
        if(buf.size() < default_bufsize / 2) {
            grow_to(default_bufsize);
        } else {
            grow_to(buf.size() * 2);
        }
    }
    buf[bufsize] = c;
    ++bufsize;
}

void Bytes::append(const char *begin, const char *end) {
    while(begin != end) {
        append(*begin);
        ++begin;
    }
}

void Bytes::extend(size_t num_bytes) noexcept {
    const size_t target_size = bufsize + num_bytes;
    grow_to(target_size);
    bufsize += num_bytes;
}

void Bytes::shrink(size_t num_bytes) noexcept {
    if(num_bytes > bufsize) {
        bufsize = 0;
    } else {
        bufsize -= num_bytes;
    }
}

void Bytes::resize_to(size_t num_bytes) noexcept {
    if(num_bytes < bufsize) {
        bufsize = num_bytes;
    } else if(num_bytes > bufsize) {
        grow_to(num_bytes);
        bufsize = num_bytes;
    }
}

Bytes &Bytes::operator=(const Bytes &o) noexcept {
    if(this != &o) {
        grow_to(o.size());
        memcpy(buf.get(), o.buf.get(), o.size());
        bufsize = o.bufsize;
    }
    return *this;
}

Bytes &Bytes::operator+=(const Bytes &o) noexcept {
    if(this == &o) {
        // FIXME, make fast.
        Bytes tmp{o};
        return *this += tmp;
    }
    grow_to(bufsize + o.bufsize);
    memcpy(buf.get() + bufsize, o.buf.get(), o.bufsize);
    bufsize += o.bufsize;
    return *this;
}

char Bytes::front() const {
    if(is_empty()) {
        throw PyException("Buffer underrun.");
    }
    return buf[0];
}

char Bytes::back() const {
    if(is_empty()) {
        throw PyException("Buffer underrun.");
    }
    auto result = buf[buf.size_bytes() - 1];
    return result;
}

void Bytes::remove(size_t from, size_t to) {
    if(from > to) {
        throw PyException("Invalid sequence to remove.");
    }
    if(from == to) {
        return;
    }
    const auto bytes_to_remove = to - from;
    const auto bytes_to_copy = size() - from - bytes_to_remove;
    memmove(buf.get() + from, buf.get() + to, bytes_to_copy);
    bufsize -= bytes_to_remove;
}

CStringView::CStringView(const char *str) noexcept : buf{str}, bufsize{strlen(str)} {}

CStringView::CStringView(const char *str, size_t length) {
    if(length == (size_t)-1) {
        length = str ? strlen(str) : 0;
    }
    // FIXME, check embedded nulls.
    buf = str;
    bufsize = length;
}

CStringView::CStringView(const char *start, const char *stop) {
    if(stop < start) {
        throw PyException("Bad range to CStringView.");
    }
    buf = start;
    bufsize = stop - start;
}

CStringView::CStringView(const CString &str) noexcept {
    buf = str.data();
    bufsize = str.size();
}

bool CStringView::operator==(const char *str) const {
    for(size_t i = 0; i < bufsize; ++i) {
        if(str[i] == '\0') {
            return false;
        }
        if(str[i] != buf[i]) {
            return false;
        }
    }
    return str[bufsize] == '\0';
}

bool CStringView::operator==(CStringView o) const {
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

char CStringView::front() const {
    if(is_empty()) {
        throw "Front requested on empty string.";
    }
    return buf[0];
}

bool CStringView::starts_with(const char *str) const {
    for(size_t i = 0; i < bufsize; ++i) {
        if(str[i] == '\0') {
            return true;
        }
        if(str[i] != buf[i]) {
            return false;
        }
    }
    return str[bufsize] == '\0';
}

bool CStringView::starts_with(CStringView view) const {
    if(view.size() > size()) {
        return false;
    }
    for(size_t i = 0; i < bufsize; ++i) {
        if(view[i] != buf[i]) {
            return false;
        }
    }
    return true;
}

size_t CStringView::find(char c) const {
    for(size_t i = 0; i < bufsize; ++i) {
        if(buf[i] == c) {
            return i;
        }
    }
    return (size_t)-1;
}

CStringView CStringView::substr(size_t pos, size_t count) const {
    if(pos >= bufsize) {
        throw PyException("CStringView index out of bounds.");
    }
    size_t actual_count = count;
    if(count == (size_t)-1) {
        actual_count = bufsize - pos;
    } else if(pos + count > bufsize) {
        actual_count = bufsize - pos;
    }
    return CStringView(buf + pos, actual_count);
}

size_t CStringView::find(CStringView substr) const {
    if(substr.size() > size()) {
        return -1;
    }
    for(size_t start = 0; start < size() - substr.size(); ++start) {
        bool has_mismatch = false;
        for(size_t i = 0; i < substr.size(); ++i) {
            if(buf[start + i] != substr.buf[i]) {
                has_mismatch = true;
                break;
            }
        }
        if(!has_mismatch) {
            return start;
        }
    }
    return (size_t)-1;
}

bool CStringView::operator<(const CStringView &o) const {
    const size_t common_size = o.size() < size() ? o.size() : size();
    auto rc = strncmp(data(), o.data(), common_size);
    if(rc < 0) {
        return true;
    }
    if(rc > 0) {
        return false;
    }
    return size() < o.size();
}

bool CStringView::overlaps(CStringView &other) const {
    const auto s1 = buf;
    const auto e1 = buf + bufsize;
    const auto s2 = other.buf;
    const auto e2 = other.buf + other.bufsize;

    if(e2 < s1) {
        return false;
    }
    if(e1 < s2) {
        return false;
    }
    return true;
}

CString CStringView::upper() const noexcept {
    CString result;
    result.reserve(size());
    for(const char c : *this) {
        result += (char)toupper((const unsigned char)c);
    }
    return result;
}

CString CStringView::lower() const noexcept {
    CString result;
    result.reserve(size());
    for(const char c : *this) {
        result += (char)tolower((const unsigned char)c);
    }
    return result;
}

CString::CString(Bytes incoming) {
    bytes = move(incoming);
    bytes.append('\0');
    check_embedded_nuls();
}

CString::CString(const CStringView &view) : CString(view.data(), view.size()) {}

CString::CString(const char *txt, size_t txtsize) {
    if(txtsize == (size_t)-1) {
        bytes = Bytes(txt, strlen(txt) + 1);
    } else if(txtsize == 0) {
        assert(bytes.is_empty());
        bytes.append('\0');
    } else {
        bytes = Bytes(txt, txtsize);
        bytes.append('\0');
        check_embedded_nuls();
    }
}

void CString::check_embedded_nuls() const {
    for(size_t i = 0; i < bytes.size() - 1; ++i) {
        if(bytes[i] == '\0') {
            throw PyException("Embedded null in CString contents.");
        }
    }
}

void CString::strip() {
    assert(!bytes.is_empty());
    const char *const start = bytes.data();
    const char *const end = bytes.data() + bytes.size();
    size_t trailing_whitespace = 0;
    size_t leading_whitespace = 0;
    for(auto *it = start; it < end && is_ascii_whitespace(*it); ++it) {
        ++leading_whitespace;
    }
    if(leading_whitespace == size()) {
        // All whitespace string.
        bytes.clear();
        bytes.append('\0');
        return;
    }
    for(auto *it = end - 2; it >= start && is_ascii_whitespace(*it); --it) {
        ++trailing_whitespace;
    }
    if(trailing_whitespace > 0) {
        bytes.pop_back();
        bytes.pop_back(trailing_whitespace);
        bytes.append('\0');
    }
    bytes.pop_front(leading_whitespace);
}

size_t CString::size() const {
    auto s = bytes.size();
    assert(s > 0);
    return s - 1;
}

CString CString::substr(size_t offset, size_t length) const {
    if(length == (size_t)-1) {
        CString(c_str() + offset, size() - offset);
    }
    // FIXME, validate range.
    return CString(c_str() + offset, length);
}

template<> Vector<CString> CString::split() const {
    auto cb_lambda = [](const CStringView &piece, void *ctx) -> bool {
        auto *arr = static_cast<Vector<CString> *>(ctx);
        arr->push_back(CString{piece});
        return true;
    };
    Vector<CString> arr;
    split(cb_lambda, static_cast<void *>(&arr));

    return arr;
}

template<> Vector<CString> CString::split_by(char c) const {
    auto cb_lambda = [](const CStringView &piece, void *ctx) -> bool {
        auto *arr = static_cast<Vector<CString> *>(ctx);
        arr->push_back(CString{piece});
        return true;
    };
    Vector<CString> arr;
    split_by(c, cb_lambda, static_cast<void *>(&arr));

    return arr;
}

void CString::split(CStringViewCallback cb, void *ctx) const {
    size_t i = 0;
    while(i < size()) {
        while(i < size() && is_ascii_whitespace(bytes[i])) {
            ++i;
        }
        if(i == size()) {
            break;
        }
        const auto string_start = i;
        ++i;
        while(i < size() && (!is_ascii_whitespace(bytes[i]))) {
            ++i;
        }
        CStringView piece{c_str() + string_start, i - string_start};
        if(!cb(piece, ctx)) {
            break;
        }
    }
}

void CString::split_by(char c, CStringViewCallback cb, void *ctx) const {
    size_t i = 0;
    while(i < size()) {
        while(i < size() && bytes[i] == c) {
            ++i;
        }
        if(i == size()) {
            break;
        }
        const auto string_start = i;
        ++i;
        while(i < size() && (bytes[i] != c)) {
            ++i;
        }
        CStringView piece{c_str() + string_start, i - string_start};
        if(!cb(piece, ctx)) {
            break;
        }
    }
}

bool CString::operator==(const char *str) const { return strcmp(str, c_str()) == 0; }

bool CString::operator==(const CStringView &o) const { return this->view() == o; }

CString &CString::operator+=(const CString &o) {
    if(this == &o) {
        // FIXME make fast.
        CString copy(o);
        return *this += copy;
    }
    bytes.pop_back();
    bytes += o.bytes;
    return *this;
}

CString &CString::operator+=(char c) {
    bytes.pop_back();
    bytes.append(c);
    bytes.append('\0');
    return *this;
}

CString &CString::operator+=(const char *str) {
    CStringView v(str);
    *this += v;
    return *this;
}

CString &CString::operator=(const char *str) {
    CStringView tmp(str);
    clear();
    (*this) = tmp;
    return *this;
}

void CString::append(const char c) {
    if(c == '\0') {
        throw PyException("Tried to add a null byte to a CString.");
    }
    bytes.pop_back();
    bytes.append(c);
    bytes.append('\0');
}

void CString::append(const char *str, size_t strsize) {
    if(strsize == (size_t)-1) {
        strsize = strlen(str);
    }
    for(size_t i = 0; i < strsize; ++i) {
        // FIXME, slow.
        append(str[i]);
    }
}

void CString::append(const char *start, const char *stop) { append(start, stop - start); }

CStringView CString::view() const { return CStringView{bytes.data(), bytes.size() - 1}; }

char CString::back() const {
    if(bytes.size() < 2) {
        throw PyException("Called back() on empty string.");
    }
    auto last_char = bytes[bytes.size() - 2];
    return last_char;
}

void CString::pop_back() noexcept {
    bytes.pop_back();
    bytes.pop_back();
    bytes.append('\0');
}

void CString::insert(size_t i, const CStringView &v) noexcept {
    if(view_points_to_this(v)) {
        CString string_copy(v);
        insert(i, string_copy.view());
        return;
    }
    bytes.insert(i, v.data(), v.size());
}

size_t CString::find(CStringView substr) const { return view().find(substr); }

size_t CString::rfind(const char c) const noexcept {
    size_t i = bytes.size() - 1;
    while(i != (size_t)-1) {
        if(bytes[i] == c) {
            return i;
        }
        --i;
    }
    return -1;
}

bool CString::view_points_to_this(const CStringView &v) const {
    return bytes.is_ptr_within(v.data());
}

uint32_t ValidatedU8Iterator::operator*() {
    compute_char_info();
    return char_info.codepoint;
}

ValidatedU8Iterator &ValidatedU8Iterator::operator++() {
    compute_char_info();
    buf += char_info.byte_count;
    has_char_info = false;
    return *this;
}

ValidatedU8Iterator ValidatedU8Iterator::operator++(int) {
    compute_char_info();
    ValidatedU8Iterator rval{*this};
    ++(*this);
    return rval;
}

ValidatedU8Iterator &ValidatedU8Iterator::operator--() {
    // FIXME, it is possible to decrement past the beginning.
    const uint8_t CONTINUATION_MASK = 0b11000000;
    const uint8_t CONTINUATION_VALUE = 0b10000000;
    --buf;
    while((*buf & CONTINUATION_MASK) == CONTINUATION_VALUE) {
        --buf;
    }
    has_char_info = false;
    return *this;
}

bool ValidatedU8Iterator::operator==(const ValidatedU8Iterator &o) const { return buf == o.buf; }

bool ValidatedU8Iterator::operator!=(const ValidatedU8Iterator &o) const { return !(*this == o); }

uint32_t ValidatedU8ReverseIterator::operator*() {
    compute_char_info();
    return char_info.codepoint;
}

ValidatedU8ReverseIterator &ValidatedU8ReverseIterator::operator++() {
    go_backwards();
    return *this;
}

ValidatedU8ReverseIterator ValidatedU8ReverseIterator::operator++(int) {
    compute_char_info();
    ValidatedU8ReverseIterator rval{*this};
    ++(*this);
    return rval;
}

bool ValidatedU8ReverseIterator::operator==(const ValidatedU8ReverseIterator &o) const {
    return original_str == o.original_str && offset == o.offset;
}

bool ValidatedU8ReverseIterator::operator!=(const ValidatedU8ReverseIterator &o) const {
    return !(*this == o);
}

void ValidatedU8ReverseIterator::go_backwards() {
    const uint8_t CONTINUATION_MASK = 0b11000000;
    const uint8_t CONTINUATION_VALUE = 0b10000000;
    if(offset < 0) {
        assert(!has_char_info);
        return;
    }
    has_char_info = false;
    if(offset == 0) {
        --offset;
        return;
    }
    --offset;
    while((original_str[offset] & CONTINUATION_MASK) == CONTINUATION_VALUE) {
        --offset;
    }
}

void ValidatedU8ReverseIterator::compute_char_info() {
    if(has_char_info) {
        return;
    }
    if(offset >= 0) {
        char_info = extract_one_codepoint(original_str + offset);
        has_char_info = true;
    }
};

CStringView U8StringView::raw_view() const {
    return CStringView{(const char *)start.byte_location(),
                       (size_t)(end.byte_location() - start.byte_location())};
}

bool U8StringView::overlaps(const U8StringView &o) const {
    auto cv1 = raw_view();
    auto cv2 = o.raw_view();
    return cv1.overlaps(cv2);
}

bool U8StringView::operator==(const char *txt) const {
    const unsigned char *data_start = start.byte_location();
    const unsigned char *data_end = end.byte_location();
    size_t i = 0;
    while(true) {
        if(data_start + i > data_end) {
            return false;
        }
        if(data_start + i == data_end) {
            return txt[i] == '\0';
        }
        if(data_start[i] != txt[i]) {
            return false;
        }
        ++i;
    }
}

U8String U8StringView::upper() const {
    U8String result;
    result.reserve(size_bytes());
    for(auto it = start; it != end; ++it) {
        const auto uppered = uppercase_unicode(*it);
        for(size_t i = 0; i < 3; ++i) {
            if(uppered.codepoints[i] != 0) {
                result.append_codepoint(uppered.codepoints[i]);
            }
        }
    }
    return result;
}

U8String U8StringView::lower() const {
    U8String result;
    result.reserve(size_bytes());
    for(auto it = start; it != end; ++it) {
        const auto lowered = lowercase_unicode(*it);
        for(size_t i = 0; i < 3; ++i) {
            if(lowered.codepoints[i] != 0) {
                result.append_codepoint(lowered.codepoints[i]);
            }
        }
    }
    return result;
}

U8String::U8String(Bytes incoming) {
    if(!is_valid_utf8(incoming.data(), incoming.size())) {
        throw PyException("Invalid UTF-8.");
    }
    cstring = CString(move(incoming));
}

U8String::U8String(const U8StringView &view) {
    if(view.start.buf >= view.end.buf) {
        throw PyException("Invalid UTF-8 string view.");
    }
    cstring = CString((const char *)view.start.buf, view.end.buf - view.start.buf);
}

U8String::U8String(const char *txt, size_t txtsize) {
    if(txtsize == (size_t)-1) {
        txtsize = strlen(txt);
    }
    if(!is_valid_utf8(txt, txtsize)) {
        throw PyException(U8String{"Invalid UTF-8."});
    }
    cstring = CString(txt, txtsize);
}

U8String U8String::substr(size_t offset, size_t length) const {
    // FIXME, validate range.
    return U8String(cstring.data() + offset, length);
}

U8StringView U8String::view() const { return U8StringView{cbegin(), cend()}; }

bool U8String::operator==(const char *str) const { return strcmp(str, cstring.c_str()) == 0; }

U8String &U8String::operator+=(const U8String &o) {
    cstring += o.cstring;
    return *this;
}

template<> Vector<U8String> U8String::split_ascii() const {
    auto cb_lambda = [](const U8StringView &piece, void *ctx) -> bool {
        auto *arr = static_cast<Vector<U8String> *>(ctx);
        arr->push_back(U8String{piece});
        return true;
    };
    Vector<U8String> arr;
    split(cb_lambda, is_ascii_whitespace, static_cast<void *>(&arr));
    return arr;
}

void U8String::split(U8StringViewCallback cb, IsSplittingCharacter is_split_char, void *ctx) const {
    ValidatedU8Iterator i = cbegin();
    const ValidatedU8Iterator end = cend();
    while(i != end) {
        while(i != end && is_split_char(*i)) {
            ++i;
        }
        if(i == end) {
            break;
        }
        const auto string_start = i;
        ++i;
        while(i != end && (!is_split_char(*i))) {
            ++i;
        }
        U8StringView piece{string_start, i};
        if(!cb(piece, ctx)) {
            break;
        }
    }
}

void U8String::insert(const ValidatedU8Iterator &it, U8StringView view) {
    cstring.insert(it.byte_location() - (const unsigned char *)cstring.data(), view.raw_view());
}

void U8String::remove(U8StringView view) {
    if(!contains(view)) {
        throw PyException("Section to remove is not contained within string.");
    }
    const size_t start_index = (const char *)view.start.byte_location() - cstring.c_str();
    const size_t end_index = (const char *)view.end.byte_location() - cstring.c_str();
    cstring.remove(start_index, end_index);
}

void U8String::pop_front() noexcept {
    if(is_empty()) {
        return;
    }
    auto start = cbegin();
    auto end = start;
    ++end;
    remove(U8StringView(start, end));
}

void U8String::pop_back() noexcept {
    if(is_empty()) {
        return;
    }
    auto end = cend();
    auto start = end;
    --start;
    remove(U8StringView(start, end));
}

void U8String::append_codepoint(uint32_t codepoint) {
    unsigned char buf[5] = {0, 0, 0, 0, 0};
    if(codepoint < 0x80) {
        buf[0] = codepoint;
    } else if(codepoint < 0x800) {
        buf[0] = 0xC0 | (codepoint >> 6 & 0x1F);
        buf[1] = 0x80 | (codepoint & 0x3F);
    } else if(codepoint < 0x10000) {
        buf[0] = 0xE0 | (codepoint >> 12 & 0x0F);
        buf[1] = 0x80 | (codepoint >> 6 & 0x3F);
        buf[2] = 0x80 | (codepoint & 0x3F);
    } else if(codepoint < 0x110000) {
        buf[0] = 0xF0 | (codepoint >> 18 & 0x07);
        buf[1] = 0x80 | (codepoint >> 12 & 0x3F);
        buf[2] = 0x80 | (codepoint >> 6 & 0x3F);
        buf[3] = 0x80 | (codepoint & 0x3F);
    } else {
        throw PyException("Unicode codepoint > 0x110000.");
    }
    cstring += (char *)buf;
}

bool U8String::contains(const U8StringView &view) const {
    return view.start.byte_location() >= (const unsigned char *)cstring.c_str() &&
           view.end.byte_location() <= (const unsigned char *)(cstring.c_str() + cstring.size());
}

bool U8String::overlaps(const U8StringView &o) const {
    const auto v1 = view();
    return v1.overlaps(o);
}

void ValidatedU8Iterator::compute_char_info() {
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
                if(!b.is_empty()) {
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

Range::Range(int64_t end_) : Range(0, end_) {}

Range::Range(int64_t start, int64_t end_) : Range(start, end_, 1) {}

Range::Range(int64_t start, int64_t end_, int64_t step_) : i{start}, end{end_}, step{step_} {}

Optional<int64_t> Range::next() {
    if(i >= end) {
        return {};
    }

    auto result = Optional<int64_t>{i};
    i += step;
    return result;
}

MMapping::~MMapping() {
    if(buf) {
        if(munmap((void *)buf, bufsize) != 0) {
            perror(nullptr);
        }
    }
}

Optional<MMapping> mmap_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if(!f) {
        return Optional<MMapping>();
    }
    fseek(f, 0, SEEK_END);
    const auto bufsize = ftell(f);
    if(bufsize == 0) {
        fclose(f);
        return Optional<MMapping>();
    }
    fseek(f, 0, SEEK_SET);
    int fd = fileno(f);
    auto *buf = mmap(nullptr, bufsize, PROT_READ, MAP_PRIVATE, fd, 0);
    fclose(f);
    if(buf == MAP_FAILED) {
        return Optional<MMapping>();
    }
    return Optional<MMapping>(MMapping(buf, bufsize));
}

CString cformat(const char *format, ...) {
    CString result;
    va_list args;
    va_start(args, format);
    auto converter = [](const char *buf, size_t bufsize, void *ctx) {
        auto *s = reinterpret_cast<CString *>(ctx);
        *s = CString(buf, bufsize);
    };
    cformat_with_cb(converter, &result, format, args);
    va_end(args);
    return result;
}

#include <errno.h>

void cformat_with_cb(StringCFormatCallback cb, void *ctx, const char *format, va_list ap) {
    const int bufsize = 1024;
    char buf[bufsize];
    auto rc = vsnprintf(buf, bufsize, format, ap);
    if(rc < 0) {
        throw PyException(strerror(errno));
    } else if(rc >= bufsize) {
        if(INT_MAX - 1 >= rc) {
            // Almost certainly either a bug or an exploit.
            internal_failure("Tried to format a buffer that is too big.");
        }
        unique_arr<char> bigbuf(bufsize + 1);
        rc = vsnprintf(buf, bufsize, format, ap);
        assert(rc > 0);
        cb(bigbuf.get(), rc, ctx);
    } else {
        cb(buf, rc, ctx);
    }
}

double clamp(double val, double lower, double upper) {
    if(isnan(val) || isnan(lower) || isnan(upper)) {
        return NAN;
    }
    if(lower > upper) {
        internal_failure("Bad range to clamp.");
    }
    if(val < lower) {
        return lower;
    }
    if(val > upper) {
        return upper;
    }
    return val;
}

int64_t clamp(int64_t val, int64_t lower, int64_t upper) {
    if(lower > upper) {
        internal_failure("Bad range to clamp.");
    }
    if(val < lower) {
        return val;
    }
    if(val > upper) {
        return upper;
    }
    return val;
}

UnicodeConversionResult uppercase_unicode(uint32_t codepoint) {
    if(codepoint < 128) {
        UnicodeConversionResult r;
        memset(&r, 0, sizeof(UnicodeConversionResult));
        r.codepoints[0] = toupper(codepoint);
        return r;
    }
    auto mit = lower_bound(
        uppercasing_multi,
        uppercasing_multi + NUM_UPPERCASING_MULTICHAR_ENTRIES,
        codepoint,
        [](const UnicodeConversionMultiChar &trial, uint32_t cp) { return trial.codepoint < cp; });
    if(mit != uppercasing_multi + NUM_UPPERCASING_MULTICHAR_ENTRIES) {
        if(mit->codepoint == codepoint) {
            return mit->converted;
        }
    }

    UnicodeConversionResult r;
    memset(&r, 0, sizeof(UnicodeConversionResult));
    auto sit = lower_bound(uppercasing_single,
                           uppercasing_single + NUM_UPPERCASING_SINGLECHAR_ENTRIES,
                           codepoint,
                           [](const UnicodeConversionSingleChar &trial, uint32_t cp) {
                               return trial.from_codepoint < cp;
                           });
    if(sit != uppercasing_single + NUM_UPPERCASING_SINGLECHAR_ENTRIES) {
        if(sit->from_codepoint == codepoint) {
            r.codepoints[0] = sit->to_codepoint;
            return r;
        }
    }
    // Letter has no upper case form, return itself.
    r.codepoints[0] = codepoint;
    return r;
}

UnicodeConversionResult lowercase_unicode(uint32_t codepoint) {
    if(codepoint < 128) {
        UnicodeConversionResult r;
        memset(&r, 0, sizeof(UnicodeConversionResult));
        r.codepoints[0] = tolower(codepoint);
        return r;
    }

    static_assert(NUM_LOWERCASING_MULTICHAR_ENTRIES == 1);
    if(lowercasing_multi[0].codepoint == codepoint) {
        return lowercasing_multi[0].converted;
    }
    UnicodeConversionResult r;
    memset(&r, 0, sizeof(UnicodeConversionResult));
    auto sit = lower_bound(lowercasing_single,
                           lowercasing_single + NUM_LOWERCASING_SINGLECHAR_ENTRIES,
                           codepoint,
                           [](const UnicodeConversionSingleChar &trial, uint32_t cp) {
                               return trial.from_codepoint < cp;
                           });
    if(sit != lowercasing_single + NUM_LOWERCASING_SINGLECHAR_ENTRIES) {
        if(sit->from_codepoint == codepoint) {
            r.codepoints[0] = sit->to_codepoint;
            return r;
        }
    }
    // Letter has no lower case form, return itself.
    r.codepoints[0] = codepoint;
    return r;
}

} // namespace pystd2025
