// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2025.hpp>

namespace pystd2025 {

struct UnicodeConversionMultiChar {
    uint32_t codepoint;
    UnicodeConversionResult converted;
};

struct UnicodeConversionSingleChar {
    uint32_t from_codepoint;
    uint32_t to_codepoint;
};

#define NUM_UPPERCASING_MULTICHAR_ENTRIES 102

extern const UnicodeConversionMultiChar uppercasing_multi[NUM_UPPERCASING_MULTICHAR_ENTRIES];

#define NUM_UPPERCASING_SINGLECHAR_ENTRIES 1397

extern const UnicodeConversionSingleChar uppercasing_single[NUM_UPPERCASING_SINGLECHAR_ENTRIES];

// clang-format off
#define NUM_LOWERCASING_MULTICHAR_ENTRIES 1
extern const UnicodeConversionMultiChar lowercasing_multi[NUM_LOWERCASING_MULTICHAR_ENTRIES];

#define NUM_LOWERCASING_SINGLECHAR_ENTRIES 1406
extern const UnicodeConversionSingleChar lowercasing_single[NUM_LOWERCASING_SINGLECHAR_ENTRIES];


} // namespace pystd2025
