// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd.hpp>
#include <stdlib.h>

void *operator new(size_t count) { return malloc(count); }

void operator delete(void *ptr, size_t count) noexcept { free(ptr); }
