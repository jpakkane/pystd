// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd_testconfig.hpp>
#include <pystd2026/statistics.hpp>

namespace {

int breakpoint_opportunity(int number) { return number; }

} // namespace

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_mean() {
    TEST_START;
    ::pystd2026::Vector<int> data;
    data.push_back(2);
    data.push_back(4);
    data.push_back(6);

    ASSERT(!::pystd2026::mean(data.begin(), data.begin()));

    ASSERT(::pystd2026::mean(data.span()) == 4);

    return 0;
}

int test_geometric_mean() {
    TEST_START;
    ::pystd2026::Vector<double> data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);

    const double expected = 1.817120592;

    auto var = ::pystd2026::geometric_mean(data.span());
    ASSERT(var);
    ASSERT(fabs(var.value() - expected) < 0.01);

    return 0;
}

int test_harmonic_mean() {
    TEST_START;
    ::pystd2026::Vector<double> data;
    data.push_back(1);
    data.push_back(2);
    data.push_back(3);

    const double expected = 1.6363636;

    auto var = ::pystd2026::harmonic_mean(data.span());
    ASSERT(var);
    ASSERT(fabs(var.value() - expected) < 0.01);

    return 0;
}

int test_variance() {
    TEST_START;
    ::pystd2026::Vector<double> data;
    data.push_back(1.0);
    data.push_back(2.0);
    data.push_back(3.0);
    data.push_back(4.0);

    const double expected = 1.66666;

    auto var = ::pystd2026::variance(data.span());
    ASSERT(var);
    ASSERT(fabs(var.value() - expected) < 0.01);

    return 0;
}

int test_covariance() {
    TEST_START;
    const int NUM_ITEMS = 9;
    double xdata[NUM_ITEMS] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double ydata[NUM_ITEMS] = {1, 2, 3, 1, 2, 3, 1, 2, 3};

    ::pystd2026::Span x(xdata, NUM_ITEMS);
    ::pystd2026::Span y(ydata, NUM_ITEMS);

    const double expected = 0.75;

    auto var = ::pystd2026::covariance(x, y);
    ASSERT(var);
    ASSERT(fabs(var.value() - expected) < 0.01);

    return 0;
}

int test_correlation() {
    TEST_START;
    const int NUM_ITEMS = 9;
    double xdata[NUM_ITEMS] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    double ydata[NUM_ITEMS] = {1, 2, 3, 1, 2, 3, 1, 2, 3};

    ::pystd2026::Span x(xdata, NUM_ITEMS);
    ::pystd2026::Span y(ydata, NUM_ITEMS);

    const double expected = 0.3162277660;

    auto var = ::pystd2026::correlation(x, y);
    ASSERT(var);
    ASSERT(fabs(var.value() - expected) < 0.01);

    return 0;
}

int test_regression() {
    TEST_START;
    const int NUM_ITEMS = 5;
    double xdata[NUM_ITEMS] = {1971, 1975, 1979, 1982, 1983};
    double ydata[NUM_ITEMS] = {1, 2, 3, 4, 5};

    ::pystd2026::Span x(xdata, NUM_ITEMS);
    ::pystd2026::Span y(ydata, NUM_ITEMS);

    const double expected_slope = 0.312;
    const double expected_intercept = -610.18;

    auto var = ::pystd2026::linear_regression(x, y);
    ASSERT(var);
    auto &v = var.value();
    ASSERT(fabs(v.slope - expected_slope) < 0.01);
    ASSERT(fabs(v.intercept - expected_intercept) < 0.01);

    return 0;
}

int test_stats() {
    printf("Testing file access.\n");
    int failing_subtests = 0;
    failing_subtests += test_mean();
    failing_subtests += test_geometric_mean();
    failing_subtests += test_harmonic_mean();
    failing_subtests += test_variance();
    failing_subtests += test_covariance();
    failing_subtests += test_regression();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_stats();
    } catch(const pystd2026::PyException &e) {
        printf("Testing failed: %s\n", e.what().c_str());
        return 42;
    } catch(...) {
        printf("Testing failed due to unknown exception.\n");
        return 43;
    }

    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
