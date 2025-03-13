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

Bytes::Bytes() noexcept : buf{new char[default_bufsize], default_bufsize} { strsize = 0; }

Bytes::Bytes(size_t initial_size) noexcept : buf{new char[initial_size], initial_size} {
    strsize = 0;
}

void Bytes::double_size() {
    const auto new_capacity = 2 * buf.size();
    unique_arr<char> new_buf{new char[new_capacity], new_capacity};
    for(size_t i = 0; i < strsize; ++i) {
        new_buf[i] = buf[i];
    }
    buf = move(new_buf);
}

void Bytes::append(const char c) {
    if(strsize >= buf.size()) {
        double_size();
    }
    buf[strsize] = c;
    ++strsize;
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
            b.append('\0');
            return b;
        }
    }
}

} // namespace pystd
