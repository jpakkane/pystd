// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#include <stdlib.h>

/*
 * These two are needed when building a shared library but not
 * with a static one. For now support only static.
 */
/*
void *operator new(size_t count) { return malloc(count); }

void operator delete(void *ptr) noexcept { free(ptr); }
void *operator new(size_t, void *ptr) noexcept { return ptr; }
*/

#ifndef __GLIBCXX__
void *operator new(size_t, void *ptr) noexcept { return ptr; }
#endif
