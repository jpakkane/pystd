// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

class U8Regex {
public:
    explicit U8Regex(const U8String &pattern);
    ~U8Regex();

    U8Regex() = delete;
    U8Regex(const U8Regex &o) = delete;
    U8Regex &operator=(const U8Regex &o) = delete;

    void *h() const { return handle; }

private:
    void *handle;
};

class U8Match {
public:
    U8Match(void *h, const U8String *orig) : handle{h}, original{orig} {};
    ~U8Match();

    U8Match() = delete;
    U8Match(U8Match &&o) : handle(o.handle), original{o.original} {
        o.handle = nullptr;
        o.original = nullptr;
    }
    U8Match &operator=(const U8Match &o) = delete;

    void *h() const { return handle; }

    U8String get_submatch(size_t i);

    size_t num_groups() const;

    U8StringView group(size_t group_num) const;

private:
    void *handle;
    const U8String *original; // Not very safe...
};

U8Match regex_search(const U8Regex &pattern, const U8String &text);

} // namespace pystd2026
