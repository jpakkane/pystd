// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<BasicIterator It> void introsort(It begin, It end) {
    pystd2026::insertion_sort(begin, end);
}

template<typename T> void introsort(pystd2026::Span<T> span) {
    introsort(span.begin(), span.end());
}

} // namespace pystd2026
