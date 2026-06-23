// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_shutil.hpp>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

namespace pystd2026 {

namespace {

int recursive_rmtree(int dir_fd, const Path &dir_path) {
    int total_failures = 0;
    DIR *d = fdopendir(dir_fd);
    if(!d) {
        throw_errno_error();
    }
    pystd2026::unique_ptr<DIR, DirCloser> dc(d);
    struct dirent *dentry;

    // Recurse first.
    while(true) {
        errno = 0;
        dentry = readdir(d);
        if(dentry == nullptr) {
            if(errno == 0) {
                break;
            } else {
                throw_errno_error();
            }
        }
        if(dentry->d_type == DT_DIR) {
            if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0) {
                continue;
            }
            int subdir_fd =
                openat(dir_fd, dentry->d_name, O_NOFOLLOW | O_DIRECTORY | O_RDONLY | O_CLOEXEC);
            if(subdir_fd < 0) {
                throw_errno_error();
            }
            FileDescriptor sd(subdir_fd);
            Path subdir_path = dir_path / dentry->d_name;
            recursive_rmtree(sd.get(), subdir_path);
        }
    }

    // Delete second.
    rewinddir(d);
    while(true) {
        errno = 0;
        dentry = readdir(d);
        if(dentry == nullptr) {
            if(errno == 0) {
                break;
            } else {
                throw_errno_error();
            }
        }
        if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0) {
            continue;
        }
        const int unlinkflags = dentry->d_type == DT_DIR ? AT_REMOVEDIR : 0;
        auto rc = unlinkat(dir_fd, dentry->d_name, unlinkflags);
        if(rc < 0) {
            ++total_failures;
        }
    }

    return total_failures;
}

} // namespace

int shutil_rmtree(const char *path) {
    int total_failures = 0;
    if(strcmp(path, "/") == 0) {
        throw PyException("Tried to delete file system root.");
    }
    if(strcmp(path, ".") == 0) {
        throw PyException("Tried to delete current directory. This is almost certainly a path "
                          "construction error.");
    }
    if(strcmp(path, "..") == 0) {
        throw PyException("Tried to delete parent directory. This is almost certainly a path "
                          "construction error.");
    }

    // Maybe it is not a directory at all?
    if(unlink(path) == 0) {
        return 0;
    }

    int rc = open(path, O_NOFOLLOW | O_DIRECTORY | O_RDONLY | O_CLOEXEC);
    if(rc < 0) {
        throw_errno_error(errno);
    }
    FileDescriptor root_fd(rc);
    Path root_path(path);
    total_failures += recursive_rmtree(root_fd.get(), root_path);
    total_failures += rmdir(path) == 0 ? 0 : 1;

    return total_failures;
}

} // namespace pystd2026
