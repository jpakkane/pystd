// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#pragma once

#include <pystd2025.hpp>
#include <pthread.h>

namespace pystd2025 {

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

private:
    pthread_mutex_t m;
};

template<Lockable T> class LockGuard final {
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
} // namespace pystd2025
