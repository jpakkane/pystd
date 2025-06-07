// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

void *operator new(size_t count) { return malloc(count); }

void operator delete(void *ptr) noexcept { free(ptr); }

void *operator new(size_t, void *ptr) noexcept { return ptr; }

namespace pystd2025 {

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

void SimpleHasher::feed_bytes(const char *buf, size_t bufsize) noexcept {
    for(size_t i = 0; i < bufsize; ++i) {
        value = 13 * value ^ (unsigned char)buf[i];
    }
}

Bytes::Bytes() noexcept : buf{new char[default_bufsize], default_bufsize} { bufsize = 0; }

Bytes::Bytes(size_t initial_size) noexcept : buf{new char[initial_size], initial_size} {
    bufsize = 0;
}

Bytes::Bytes(const char *data, size_t datasize) noexcept : buf{datasize} {
    for(size_t i = 0; i < datasize; ++i) {
        buf[i] = data[i];
    }
    bufsize = datasize;
}

Bytes::Bytes(Bytes &&o) noexcept {
    buf = move(o.buf);
    bufsize = o.bufsize;
    o.bufsize = 0;
}

Bytes::Bytes(const Bytes &o) noexcept {
    buf = unique_arr<char>(new char[o.buf.size()], o.buf.size());
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
    memcpy(new_buf.get(), buf.get(), bufsize);
    buf = move(new_buf);
}

void Bytes::append(const char c) {
    if(bufsize >= buf.size()) {
        grow_to(buf.size() * 2);
    }
    buf[bufsize] = c;
    ++bufsize;
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

CStringView::CStringView(const char *str) noexcept : buf{str}, bufsize{strlen(str)} {}

CStringView::CStringView(const char *str, size_t length) {
    // FIXME, check embedded nulls.
    buf = str;
    bufsize = length;
}

bool CStringView::operator==(const char *str) {
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
        throw PyException("Index out of bounds.");
    }
    size_t actual_count = count;
    if(count == (size_t)-1) {
        actual_count = bufsize - pos;
    } else if(pos + count > bufsize) {
        actual_count = bufsize - pos;
    }
    return CStringView(buf + pos, actual_count);
}

CString::CString(Bytes incoming) {
    bytes = move(incoming);
    bytes.append('\0');
    check_embedded_nuls();
}

CString::CString(const CStringView &view) : CString(view.data(), view.size()) {}

CString::CString(const char *txt, size_t txtsize) {
    if(txtsize == (size_t)-1) {
        bytes = Bytes(txt, strlen(txt));
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

bool CString::operator==(const char *str) { return strcmp(str, c_str()) == 0; }

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

void CString::append(const char c) {
    if(c == '\0') {
        throw PyException("Tried to add null byte to a CString.");
    }
    bytes.pop_back();
    bytes.append(c);
    bytes.append('\0');
}

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

CString format(const char *format, ...) {
    CString result;
    va_list args;
    va_start(args, format);
    auto converter = [](const char *buf, size_t bufsize, void *ctx) {
        auto *s = reinterpret_cast<CString *>(ctx);
        *s = CString(buf, bufsize);
    };
    format_with_cb(converter, &result, format, args);
    va_end(args);
    return result;
}

void format_with_cb(StringFormatCallback cb, void *ctx, const char *format, ...) {
    const int bufsize = 1024;
    char buf[bufsize];
    va_list args;
    va_start(args, format);
    auto rc = vsnprintf(buf, bufsize, format, args);
    if(rc < bufsize) {
        internal_failure("Temp buffer was not big enough.");
    }
    va_end(args);
    cb(buf, bufsize, ctx);
}

} // namespace pystd2025
