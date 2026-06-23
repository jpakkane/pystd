// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>
#include <stdio.h>
#include <dirent.h>

namespace pystd2026 {

struct DirCloser {
    static void del(DIR *dir) { closedir(dir); }
};

struct FileCloser {
    static void del(FILE *f) { fclose(f); }
};

class FileDescriptor {
public:
    FileDescriptor() noexcept = default;
    explicit FileDescriptor(int fdnum) noexcept : fd{fdnum} {}
    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor(FileDescriptor &&o) noexcept;

    FileDescriptor &operator=(const FileDescriptor &o) = delete;
    FileDescriptor &operator=(FileDescriptor &&o) noexcept;

    ~FileDescriptor();

    int get() const noexcept;

private:
    int fd;
};

class GlobResult;

class GlobResultInternal;

class Path {
public:
    Path() noexcept {};
    Path(const char *path);
    explicit Path(CString path);

    bool exists() const noexcept;
    bool is_file() const noexcept;
    bool is_dir() const noexcept;
    bool is_symlink() const noexcept;
    bool is_socket() const noexcept;
    bool is_fifo() const noexcept;

    bool is_abs() const;

    CString extension() const;
    Path filename() const;

    Vector<CString> split() const;

    Path operator/(const Path &o) const noexcept;
    Path operator/(const char *str) const noexcept;
    Path operator/(const CString &str) const noexcept;

    Optional<Bytes> load_bytes();
    Optional<U8String> load_text();

    bool try_write_bytes(const char *data, size_t data_size) noexcept;
    void write_bytes(const char *data, size_t data_size);

    void mkdir();

    void replace_extension(CStringView new_extension);
    void replace_extension(const char *str) { replace_extension(CStringView(str)); }

    const char *c_str() const noexcept { return buf.c_str(); }
    size_t size() const noexcept { return buf.size(); }

    bool is_empty() const noexcept { return buf.is_empty(); }

    bool rename_to(const Path &targetname) const noexcept;

    GlobResult glob(const char *pattern);

private:
    CString buf;
};

class GlobResult {
public:
    friend class Path;
    GlobResult() noexcept;
    GlobResult(GlobResult &&o) noexcept;
    ~GlobResult();
    Optional<Path> next();

    GlobResult &operator=(GlobResult &&o) noexcept = default;

private:
    GlobResult(const Path &path, const char *glob_pattern);

    unique_ptr<GlobResultInternal> p;
};

} // namespace pystd2026
