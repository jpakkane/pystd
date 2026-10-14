// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2026/common.hpp>
#include <pthread.h>

namespace pystd2026 {

template<typename T>
concept Lockable = requires(T a) {
    a.lock();
    a.unlock();
};

class Mutex final {
public:
    Mutex() noexcept;
    ~Mutex();
    Mutex(const Mutex &) = delete;
    Mutex &operator=(const Mutex &) = delete;

    void lock();
    bool try_lock();
    void unlock();

    pthread_mutex_t &native() { return m; }

private:
    pthread_mutex_t m;
};

template<::pystd2026::Lockable T> class LockGuard final {
public:
    explicit LockGuard(T &m_) : m{m_} { m.lock(); }

    ~LockGuard() { m.unlock(); }

    LockGuard(const LockGuard &) = delete;
    LockGuard &operator=(const LockGuard &) = delete;

private:
    T &m;
};

class Thread final {
public:
    Thread(void *(*thread_main_func)(void *), void *ctx);
    ~Thread();

    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    void join();
    void detach();

private:
    pthread_t thread;
    bool thread_is_finalized = false;
};

class ConditionVariable final {
public:
    ConditionVariable() noexcept;
    ~ConditionVariable();

    ConditionVariable(const ConditionVariable &) = delete;
    ConditionVariable &operator=(const ConditionVariable &) = delete;

    void signal();
    void broadcast();

    void wait(::pystd2026::Mutex &m);

private:
    pthread_cond_t cond;
};

} // namespace pystd2026
