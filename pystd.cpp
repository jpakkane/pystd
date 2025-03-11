// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd.hpp>
#include <stdlib.h>

void *operator new(size_t count) { return malloc(count); }

void operator delete(void *ptr, size_t count) noexcept { free(ptr); }

namespace pystd {

namespace {

const size_t default_bufsize = 16;

}

Bytes::Bytes() noexcept : str{new char[default_bufsize]} {
    str[0] = '\0';
    capacity = default_bufsize;
    strsize = 0;
}

void Bytes::double_size() {
    const auto new_capacity = 2 * capacity;
    unique_arr<char> new_buf{new char[new_capacity]};
    for(size_t i = 0; i <= strsize; ++i) {
        new_buf[i] = str[i];
    }
    capacity = new_capacity;
    str = move(new_buf);
}

void Bytes::append(const char c) {
    if(strsize + 2 > capacity) {
        double_size();
    }
    str[strsize] = c;
    ++strsize;
    str[strsize] = '\0';
}

File::File(const char *fname, const char *modes) : policy{EncodingPolicy::Enforce} {
    f = fopen(fname, modes);
    if(!f) {
        abort();
    }
}

File::~File() { fclose(f); }

Bytes File::readline_bytes() {
    Bytes b;
    // The world's least efficient readline.
    char c;
    while(true) {
        auto rc = fread(&c, 1, 1, f);
        if(rc != 1) {
            if(feof(f)) {
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

} // namespace pystd
