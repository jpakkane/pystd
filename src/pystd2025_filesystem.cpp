// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <sys/stat.h>

// Currently does only Posix paths.

namespace pystd2025 {

bool Path::exists() const {
    struct stat sb;
    return stat(buf.c_str(), &sb) == 0;
}

bool Path::is_file() const {
    struct stat sb;
    if(stat(buf.c_str(), &sb) == 0) {
        return (sb.st_mode & S_IFMT) == S_IFREG;
    }
    return false;
}

bool Path::is_dir() const {
    struct stat sb;
    if(stat(buf.c_str(), &sb) == 0) {
        return (sb.st_mode & S_IFMT) == S_IFDIR;
    }
    return false;
}

bool Path::is_abs() const { return (!buf.is_empty() && buf[0] == '/'); }

} // namespace pystd2025
