// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/common.hpp>

namespace pystd2026 {

class Path;

::pystd2026::File TemporaryFile();

class TemporaryDirectory {
public:
    TemporaryDirectory();
    ~TemporaryDirectory();

    ::pystd2026::Path get_path() const;

private:
    CString directory;
};

} // namespace pystd2026
