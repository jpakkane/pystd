// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/common.hpp>
#include <math.h>

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

template<WellBehaved ValueType, ::pystd2026::BasicIterator It>
ValueType variance(It begin, It end, ValueType mean) noexcept {
    ValueType summed{};
    for(auto it = begin; it != end; ++it) {
        const ValueType x = *it - mean;
        summed += x * x;
    }

    const ValueType num_elements = (end - begin);
    return summed / (num_elements - 1);
}

template<::pystd2026::BasicIterator It>
auto variance(It begin, It end) noexcept
    -> ::pystd2026::Optional<::pystd2026::remove_cvref_t<decltype(*begin)>> {
    if(end - begin < 2) {
        return {};
    }
    auto mean_value = ::pystd2026::mean(begin, end);
    if(!mean_value) {
        return {};
    }

    return ::pystd2026::variance(begin, end, mean_value.value());
}

template<WellBehaved ValueType>
::pystd2026::Optional<ValueType> variance(::pystd2026::Span<ValueType> span) {
    return ::pystd2026::variance(span.begin(), span.end());
}

} // namespace pystd2026
