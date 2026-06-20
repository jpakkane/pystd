// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <pystd2026_tempfile.hpp>
#include <stdio.h>
#include <errno.h>

namespace pystd2026 {

::pystd2026::File TemporaryFile() {
    errno = 0;
    FILE *f = tmpfile();
    if(!f) {
        if(errno == 0) {
            throw PyException("Unique file name for temp file could not be created.");
        }
        throw_errno_error(errno);
    }
    return ::pystd2026::File{f};
}

} // namespace pystd2026
