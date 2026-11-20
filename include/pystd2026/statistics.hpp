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

template<::pystd2026::BasicIterator It>
auto geometric_mean(It begin, It end) noexcept
    -> ::pystd2026::Optional<::pystd2026::remove_cvref_t<decltype(*begin)>> {
    using ValueType = ::pystd2026::remove_cvref_t<decltype(*begin)>;
    if(begin == end) {
        return ::pystd2026::Optional<ValueType>{};
    }
    ValueType summed = 1;
    ValueType N = end - begin;
    for(auto it = begin; it != end; ++it) {
        summed *= *it;
    }

    return pow(summed, 1.0 / N);
}

template<WellBehaved ValueType>
::pystd2026::Optional<ValueType> geometric_mean(::pystd2026::Span<ValueType> span) {
    return geometric_mean(span.begin(), span.end());
}

template<::pystd2026::BasicIterator It>
auto harmonic_mean(It begin, It end) noexcept
    -> ::pystd2026::Optional<::pystd2026::remove_cvref_t<decltype(*begin)>> {
    using ValueType = ::pystd2026::remove_cvref_t<decltype(*begin)>;
    if(begin == end) {
        return ::pystd2026::Optional<ValueType>{};
    }
    ValueType summed = 0;
    ValueType N = end - begin;
    for(auto it = begin; it != end; ++it) {
        if(fabs(*it) < 0.0000001) {
            return 0;
        }
        summed += 1.0 / (*it);
    }

    return N / summed;
}

template<WellBehaved ValueType>
::pystd2026::Optional<ValueType> harmonic_mean(::pystd2026::Span<ValueType> span) {
    return harmonic_mean(span.begin(), span.end());
}

template<WellBehaved ValueType, ::pystd2026::BasicIterator It>
ValueType variance(It begin, It end, ValueType mean) noexcept {
    ValueType summed{};
    for(auto it = begin; it != end; ++it) {
        const ValueType x = *it - mean;
        summed += x * x;
    }

    const ValueType N = (end - begin);
    return summed / (N - 1);
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

template<WellBehaved ValueType>
::pystd2026::Optional<ValueType> covariance(::pystd2026::Span<ValueType> x,
                                            ::pystd2026::Span<ValueType> y) noexcept {
    if(x.size() < 2 || y.size() < 2) {
        return {};
    }
    if(x.size() != y.size()) {
        return {};
    }

    const auto xmean = ::pystd2026::mean(x).value();
    const auto ymean = ::pystd2026::mean(y).value();

    const ValueType N = x.size();

    ValueType sum{};
    for(size_t i = 0; i < x.size(); ++i) {
        sum += (x[i] - xmean) * (y[i] - ymean);
    }

    return sum / (N - 1);
}

template<WellBehaved ValueType>
::pystd2026::Optional<ValueType> correlation(::pystd2026::Span<ValueType> x,
                                             ::pystd2026::Span<ValueType> y) noexcept {
    if(x.size() < 2 || y.size() < 2) {
        return {};
    }
    if(x.size() != y.size()) {
        return {};
    }

    const ValueType N = x.size();

    ValueType sum_xy{};
    ValueType sum_x{};
    ValueType sum_y{};
    ValueType sum_xx{};
    ValueType sum_yy{};
    for(size_t i = 0; i < x.size(); ++i) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xy += x[i] * y[i];
        sum_xx += x[i] * x[i];
        sum_yy += y[i] * y[i];
    }

    const ValueType numerator = N * sum_xy - sum_x * sum_y;
    const ValueType denominator1 = sqrt(N * sum_xx - sum_x * sum_x);
    const ValueType denominator2 = sqrt(N * sum_yy - sum_y * sum_y);

    return numerator / (denominator1 * denominator2);
}

template<typename T> struct RegressionResult {
    T slope;
    T intercept;
};

template<WellBehaved ValueType>
::pystd2026::Optional<RegressionResult<ValueType>>
linear_regression(::pystd2026::Span<ValueType> x, ::pystd2026::Span<ValueType> y) noexcept {
    if(x.size() < 2 || y.size() < 2) {
        return {};
    }
    if(x.size() != y.size()) {
        return {};
    }

    const ValueType N = x.size();

    ValueType sum_x{};
    ValueType sum_x2{};
    for(const auto &i : x) {
        sum_x += i;
        sum_x2 += i * i;
    }
    // Do the pseudoinverse
    ValueType XX[2][2];
    XX[0][0] = N;
    XX[1][0] = sum_x;
    XX[0][1] = sum_x;
    XX[1][1] = sum_x2;

    const ValueType D = XX[0][0] * XX[1][1] - XX[1][0] * XX[0][1];

    if(fabs(D) < 0.00001) {
        return {};
    }

    ValueType I_X[2][2];
    I_X[0][0] = XX[1][1] / D;
    I_X[1][1] = XX[0][0] / D;
    I_X[1][0] = -XX[1][0] / D;
    I_X[0][1] = -XX[0][1] / D;

    ValueType Xty[2] = {{}, {}};
    for(size_t i = 0; i < x.size(); ++i) {
        Xty[0] += y[i];
        Xty[1] += x[i] * y[i];
    }

    ValueType intercept = I_X[0][0] * Xty[0] + I_X[0][1] * Xty[1];
    ValueType slope = I_X[1][0] * Xty[0] + I_X[1][1] * Xty[1];

    return RegressionResult<ValueType>{slope, intercept};
}

} // namespace pystd2026
