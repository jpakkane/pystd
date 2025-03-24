// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once
#include <stdlib.h>

namespace pystd0 {

// This clas only exists to show how you can have multiple
// classes with the same name but different implementation
// in the same project at the same time.

class Bytes final {
public:
    Bytes() {
        buf[0] = 'a';
        buf[1] = 'b';
        buf[2] = 'c';
        buf[3] = '\0';
    }

    size_t size() const { return 3; }
    // This Bytes is always zero terminated and has a c_str.
    // The one in 2025 does neither of the two.
    // Thus we have broken both API _and_ ABI.
    // Yet everything still works. Or at least
    // results in a compiler error if you mix
    // the two.
    const char *c_str() const { return buf; }

private:
    char buf[4];
};

} // namespace pystd0
