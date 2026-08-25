// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/tempfile.hpp>
#include <pystd2026/pathlib.hpp>
#include <pystd2026/shutil.hpp>

#include <stdio.h>
#include <errno.h>

#if defined __APPLE__
#include <unistd.h>
#endif

namespace pystd2026 {

::pystd2026::File TemporaryFile() {
    errno = 0;
    FILE *f = tmpfile();
    if(!f) {
        if(errno == 0) {
            throw PyException("Unique file name for temp file could not be created.");
        }
        throw_errno_error();
    }
    return ::pystd2026::File{f};
}

TemporaryDirectory::TemporaryDirectory() {
    char dir_template[] = "/tmp/XXXXXX";
    char *dirname = mkdtemp(dir_template);
    if(!dirname) {
        throw_errno_error();
    }
    directory = dirname;
}

TemporaryDirectory::~TemporaryDirectory() {
    try {
        ::pystd2026::shutil_rmtree(directory.c_str());
    } catch(PyException &e) {
        fprintf(stderr, "%s\n", e.what().c_str());
    } catch(...) {
        fprintf(stderr, "Unknown error during temporary directory deletion.");
    }
}

Path TemporaryDirectory::get_path() const { return Path(directory.c_str()); }

} // namespace pystd2026
