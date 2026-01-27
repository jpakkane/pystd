// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026.hpp>

namespace pystd2026 {

template<WellBehaved It> void heapsort(It begin, It end) { pystd2026::insertion_sort(begin, end); }

} // namespace pystd2026
