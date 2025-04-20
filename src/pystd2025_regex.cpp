// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

namespace pystd2025 {

U8Regex::U8Regex(const U8String &pattern) {
    int errorcode = 0;
    PCRE2_SIZE erroroffset = 0;
    handle = pcre2_compile((const unsigned char *)pattern.c_str(),
                           PCRE2_ZERO_TERMINATED, // pattern.size_bytes(),
                           PCRE2_UTF,
                           &errorcode,
                           &erroroffset,
                           nullptr);
    if(!handle) {
        const PCRE2_SIZE bufsize = 160;
        PCRE2_UCHAR buf[bufsize];
        const auto strsize = pcre2_get_error_message(errorcode, buf, bufsize);
        if(strsize < 0) {
            throw PyException("Compiling regex failed.");
        }
        throw PyException(U8String((const char *)buf, strsize));
    }
}

U8Regex::~U8Regex() { pcre2_code_free((pcre2_code_8 *)handle); }

U8Match::~U8Match() {
    if(handle) {
        pcre2_match_data_free((pcre2_match_data_8 *)handle);
    }
}

U8String U8Match::get_submatch(size_t i) {
    auto osize = pcre2_get_ovector_count((pcre2_match_data_8 *)handle);
    if(i >= osize) {
        throw PyException("Submatch does not exist.");
    }
    auto ovector = pcre2_get_ovector_pointer((pcre2_match_data_8 *)handle);
    auto match_start = ovector[2 * i];
    auto match_length = ovector[2 * i + 1] - ovector[2 * i];
    return original->substr(match_start, match_length);
}

size_t U8Match::num_groups() const {
    const size_t ovector_count = pcre2_get_ovector_count((pcre2_match_data_8 *)handle);
    return ovector_count - 1;
}

U8StringView U8Match::group(size_t group_num) const {
    const size_t group_count = pcre2_get_ovector_count((pcre2_match_data_8 *)handle);
    if(group_num > group_count) {
        throw PyException("Invalid regex group number.");
    }
    const size_t *group_vector = pcre2_get_ovector_pointer((pcre2_match_data_8 *)handle);

    const size_t match_start = group_vector[2 * group_num];
    const size_t match_end = group_vector[2 * group_num + 1];
    return U8StringView{ValidatedU8Iterator{(const unsigned char *)original->c_str() + match_start},
                        ValidatedU8Iterator{(const unsigned char *)original->c_str() + match_end}};
}

U8Match regex_search(const U8Regex &pattern, const U8String &text) {
    auto match_data = pcre2_match_data_create_from_pattern((pcre2_code_8 *)pattern.h(), NULL);
    U8Match m{match_data, &text};
    auto rc = pcre2_match((pcre2_code_8 *)pattern.h(),
                          (const unsigned char *)text.c_str(),
                          PCRE2_ZERO_TERMINATED, // text.size_bytes(), // For some reason did not
                                                 // work when giving the actual length.
                          0,
                          0,
                          (pcre2_match_data_8 *)m.h(),
                          nullptr);
    if(rc < 0) { // Zero means no match.
        // const PCRE2_SIZE bufsize = 160;
        // PCRE2_UCHAR buf[bufsize];
        // pcre2_get_error_message(rc, buf, bufsize);
        // printf("%s\n", buf);
        throw PyException("Regex matching failed.");
    }
    return m;
}

} // namespace pystd2025
