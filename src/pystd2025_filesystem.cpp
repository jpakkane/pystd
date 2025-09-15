// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025.hpp>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>

// Currently does only Posix paths.

namespace pystd2025 {

namespace {

struct DirCloser {
    static void del(DIR *dir) { closedir(dir); }
};

bool glob_matches(const char *text,
                  const size_t text_offset,
                  const char *pattern,
                  const size_t pattern_offset) {
    const char text_char = text[text_offset];
    const char pattern_char = pattern[pattern_offset];
    if(text_char == '\0') {
        if(pattern_char == '\0') {
            return true;
        }
        return false;
    }
    if(pattern_char == '\0') {
        return false;
    }
    if(pattern_char == '?') {
        return glob_matches(text, text_offset + 1, pattern, pattern_offset + 1);
    } else if(pattern_char == '*') {
        if(text_offset == 0 && text_char == '.') {
            // Files with leading dots do not match star globs.
            return false;
        }
        if(pattern[pattern_offset + 1] == '\0') {
            // Pattern ends with a *, so it will match everything.
            return true;
        }
        size_t gap = 0;
        while(text[gap] != '\0') {
            if(glob_matches(text, text_offset + gap, pattern, pattern_offset + 1)) {
                return true;
            }
            ++gap;
        }
        return false;
    } else if(pattern_char == text_char) {
        return glob_matches(text, text_offset + 1, pattern, pattern_offset + 1);
    } else {
        return false;
    }
}

bool glob_matches(const char *text, const char *pattern) {
    return glob_matches(text, 0, pattern, 0);
}

void append_subdirs_of(const Path &root_, Vector<Path> &dirs) {
    Path root(root_); // Make a copy, because root_ may be in the vector and it may grow.
    DIR *d = root.is_empty() ? opendir(".") : opendir(root.c_str());
    if(!d) {
        return;
    }
    dirent *entry = readdir(d);
    unique_ptr<DIR, DirCloser> dc(d);
    while(entry) {
        // Entries with a leading dot do not match globs.
        if(entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            dirs.push_back(root / entry->d_name);
        }
        entry = readdir(d);
    }
}

Vector<Path> glob_starstar_dirs(const Path &root) {
    Vector<Path> all_subdirs;
    append_subdirs_of(root, all_subdirs);
    size_t i = 0;
    while(i < all_subdirs.size()) {
        append_subdirs_of(all_subdirs[i], all_subdirs);
        ++i;
    }
    return all_subdirs;
}

} // namespace

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

Path Path::operator/(const CString &str) const noexcept { return *this / str.c_str(); }

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

Vector<CString> Path::split() const { return buf.split_by('/'); }

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
    GlobResultInternal(const Path &path, const char *glob_pattern)
        : root_dir{path}, pattern{glob_pattern}, parts(pattern.split_by('/')) {
        bool starstar_used = false;
        for(const auto &part : parts) {
            if(part == "**") {
                if(starstar_used) {
                    throw PyException("Multiple ** operators in a glob lookup not yet supported.");
                }
                starstar_used = true;
            } else {
                if(part.find("**") != (size_t)-1) {
                    throw PyException("The ** operator must be its own full segment.");
                }
            }
        }
        DIR *d = root_dir.is_empty() ? opendir(".") : opendir(root_dir.c_str());
        if(!d) {
            throw PyException("Unknown directory.");
        }
        dir_stack.emplace_back(0, root_dir, unique_ptr<DIR, DirCloser>(d));
    }

    Optional<Path> next() {
        while(true) {
            if(dir_stack.is_empty()) {
                return {};
            }
            const bool is_last_part = dir_stack.back().part_number == parts.size() - 1;
            dirent *entry = readdir(dir_stack.back().d.get());
            if(!entry) {
                dir_stack.pop_back();
                continue;
            }
            if(entry->d_name[0] == '.') {
                if(entry->d_name[1] == '\0') {
                    continue;
                }
                if(entry->d_name[1] == '.' && entry->d_name[2] == '\0') {
                    continue;
                }
            }
            if(glob_matches(entry->d_name, parts[dir_stack.back().part_number].c_str())) {
                if(is_last_part) {
                    return dir_stack.back().path / entry->d_name;
                } else {
                    if(entry->d_type == DT_DIR) {
                        Path subdir_name = dir_stack.back().path / entry->d_name;
                        auto *sd = opendir(subdir_name.c_str());
                        if(sd) {
                            dir_stack.emplace_back(dir_stack.back().part_number + 1,
                                                   move(subdir_name),
                                                   unique_ptr<DIR, DirCloser>(sd));
                        } else {
                            // Dir can not be accessed, so ignore it. Happens e.g. when
                            // you don't have access rights or the directory was deleted
                            // before we could access it.
                        }
                    }
                }
            }
        }
    }

private:
    struct DirSearchState {
        size_t part_number;
        Path path;
        unique_ptr<DIR, DirCloser> d;
    };
    Path root_dir;
    CString pattern;
    Vector<CString> parts;
    Vector<DirSearchState> dir_stack;
};

GlobResult::GlobResult(GlobResult &&o) noexcept = default;

GlobResult::~GlobResult() = default;

GlobResult::GlobResult(const Path &path, const char *glob_pattern)
    : p(new GlobResultInternal{path, glob_pattern}) {}

Optional<Path> GlobResult::next() {
    if(!p) {
        return Optional<Path>();
    }
    return p->next();
}

} // namespace pystd2025
