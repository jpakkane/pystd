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

Path Path::operator/(const char *str) const noexcept {
    Path p(str);
    return *this / p;
}

CString Path::extension() const {
    auto loc = buf.rfind('.');
    if(loc == (size_t)-1) {
        return CString();
    }
    return buf.substr(loc);
}

Path Path::filename() const {
    auto loc = buf.rfind('/');
    if(loc == (size_t)-1) {
        return Path(buf);
    }
    return Path(buf.substr(loc + 1));
}

Optional<Bytes> Path::load_bytes() {
    FILE *f = fopen(buf.c_str(), "rb");
    if(!f) {
        return Optional<Bytes>();
    }
    fseek(f, 0, SEEK_END);
    const auto file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    Bytes contents;
    contents.resize_to(file_size);
    auto bytes_read = fread(contents.data(), 1, file_size, f);
    if(ferror(f) != 0) {
        fclose(f);
        return Optional<Bytes>();
    }
    fclose(f);
    if(bytes_read != (size_t)file_size) {
        return Optional<Bytes>();
    }
    return Optional<Bytes>(move(contents));
}

Optional<U8String> Path::load_text() {
    auto raw = load_bytes();
    if(!raw) {
        return Optional<U8String>();
    }
    try {
        U8String u(move(*raw));
        return Optional<U8String>(move(u));
    } catch(...) {
    }
    return Optional<U8String>();
}

void Path::replace_extension(CStringView new_extension) {
    // FIXME, assumes an extension can be found before
    // a path separator.
    auto extpoint = buf.rfind('.');
    if(extpoint == (size_t)-1) {
        buf += new_extension;
    } else {
        while(buf.size() > extpoint) {
            buf.pop_back();
        }
        buf += new_extension;
    }
}

bool Path::rename_to(const Path &targetname) const noexcept {
    if(targetname.is_dir()) {
        fprintf(stderr, "Can not rename a file to a directory.");
        return false;
    }
    auto rc = rename(c_str(), targetname.c_str());
    if(rc != 0) {
        perror(nullptr);
    }
    return rc == 0;
}

GlobResult Path::glob(const char *pattern) { return GlobResult(*this, pattern); }

class GlobResultInternal {
public:
    GlobResultInternal(Path &path, const char *glob_pattern)
        : root_dir{path}, pattern{glob_pattern} {}

    Optional<Path> next() { throw "Not implemented yet."; }

private:
    Path root_dir;
    CString pattern;
    Vector<CString> parts;
};

GlobResult::GlobResult(GlobResult &&o) noexcept = default;

GlobResult::~GlobResult() = default;

GlobResult::GlobResult(const Path &path, const char *glob_pattern) { throw "Not implemented yet."; }

Optional<Path> GlobResult::next() {
    if(!p) {
        return Optional<Path>();
    }
    return p->next();
}

} // namespace pystd2025
