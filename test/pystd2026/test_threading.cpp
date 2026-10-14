// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/threading.hpp>
#include <pystd_testconfig.hpp>

int breakpoint_opportunity(int number) { return number; }

#define ASSERT_WITH(statement, message)                                                            \
    if(!(statement)) {                                                                             \
        printf("%s:%d %s\n", __FILE__, __LINE__, message);                                         \
        return breakpoint_opportunity(1);                                                          \
    }

#define ASSERT(statement) ASSERT_WITH((statement), "Check failed.");

#define TEST_START printf("Test: %s\n", __PRETTY_FUNCTION__)

int test_mutex() {
    TEST_START;

    pystd2026::Mutex m;

    ASSERT(m.try_lock());
    ASSERT(!m.try_lock());
    m.unlock();

    {
        pystd2026::LockGuard<pystd2026::Mutex> guard(m);
        ASSERT(!m.try_lock());
    }
    ASSERT(m.try_lock());
    m.unlock();
    return 0;
}

struct SharedState {
    int x{0};
    ::pystd2026::Mutex m;
};

class IncrementerThread : public pystd2026::ThreadInterface {
public:
    IncrementerThread(SharedState &c) : ctx{c} {}
    virtual ~IncrementerThread() = default;

    void *run() override {
        for(int i = 0; i < 1000; ++i) {
            pystd2026::LockGuard<pystd2026::Mutex> lg(ctx.m);
            ++ctx.x;
        }
        return nullptr;
    }

private:
    SharedState &ctx;
};

int test_thread() {
    TEST_START;
    SharedState ctx;

    {
        pystd2026::Thread th1(::pystd2026::move(
            ::pystd2026::unique_ptr<::pystd2026::ThreadInterface>(new IncrementerThread{ctx})));
        pystd2026::Thread th2(
            ::pystd2026::unique_ptr<::pystd2026::ThreadInterface>(new IncrementerThread{ctx}));
        pystd2026::Thread th3(
            ::pystd2026::unique_ptr<::pystd2026::ThreadInterface>(new IncrementerThread{ctx}));
        pystd2026::Thread th4(
            ::pystd2026::unique_ptr<::pystd2026::ThreadInterface>(new IncrementerThread{ctx}));
    }
    ASSERT(ctx.x == 4000);

    return 0;
}

int test_threading() {
    printf("Testing threading.\n");
    int failing_subtests = 0;
    failing_subtests += test_mutex();
    failing_subtests += test_thread();
    return failing_subtests;
}

int main(int argc, char **argv) {
    int total_errors = 0;
    try {
        total_errors += test_threading();
    } catch(const pystd2026::PyException &e) {
        printf("Testing failed: %s\n", e.what().c_str());
        return 42;
    }

    if(total_errors) {
        printf("\n%d total errors.\n", total_errors);
    } else {
        printf("\nNo errors detected.\n");
    }
    return total_errors;
}
