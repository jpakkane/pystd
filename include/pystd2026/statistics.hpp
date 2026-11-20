// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/common.hpp>

namespace pystd2026 {

template<::pystd2026::BasicIterator It>
auto mean(It begin, It end) noexcept
    -> ::pystd2026::Optional<::pystd2026::remove_cvref_t<decltype(*begin)>> {
    using ValueType = ::pystd2026::remove_cvref_t<decltype(*begin)>;
    if(begin == end) {
        return ::pystd2026::Optional<ValueType>{};
    }
    ValueType summed{};
    for(auto it = begin; it != end; ++it) {
        summed += *it;
    }

    return summed / (end - begin);
}

template<WellBehaved ValueType>
::pystd2026::Optional<ValueType> mean(::pystd2026::Span<ValueType> span) {
    return mean(span.begin(), span.end());
}

} // namespace pystd2026
