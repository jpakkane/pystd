// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <sys/stat.h>
#include <assert.h>

// Currently does only Posix paths.

namespace pystd2025 {

Path::Path(const char *path) : buf{path} {
    while(!buf.is_empty() && buf.back() == '/') {
        buf.pop_back();
    }
}

Path::Path(CString path) : buf{move(path)} {
    while(!buf.is_empty() && buf.back() == '/') {
        buf.pop_back();
    }
}

bool Path::exists() const noexcept {
    struct stat sb;
    return stat(buf.c_str(), &sb) == 0;
}

bool Path::is_file() const noexcept {
    struct stat sb;
    if(stat(buf.c_str(), &sb) == 0) {
        return (sb.st_mode & S_IFMT) == S_IFREG;
    }
    return false;
}

bool Path::is_dir() const noexcept {
    struct stat sb;
    if(stat(buf.c_str(), &sb) == 0) {
        return (sb.st_mode & S_IFMT) == S_IFDIR;
    }
    return false;
}

bool Path::is_abs() const { return (!buf.is_empty() && buf[0] == '/'); }

Path Path::operator/(const Path &o) const noexcept {
    if(o.is_abs() || buf.is_empty()) {
        return o;
    }
    assert(buf.back() != '/');
    CString newpath = buf;
    newpath.append('/');
    newpath += o.buf;
    return Path(move(newpath));
}

} // namespace pystd2025
