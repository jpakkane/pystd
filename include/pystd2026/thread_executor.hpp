// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/threading.hpp>

namespace pystd2026 {

class ExecutorWorkerThread;

class ExecutorTaskInterface {
public:
    virtual ~ExecutorTaskInterface() = default;

    virtual void run() = 0;
};

class ThreadExecutor final {
public:
    explicit ThreadExecutor(int num_threads = -1);
    ~ThreadExecutor();

    ThreadExecutor(const ThreadExecutor &) = delete;
    ThreadExecutor &operator=(const ThreadExecutor &) = delete;

    void submit(::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface> task);

    void reserve(size_t num_tasks) noexcept;

    size_t size() const noexcept;

private:
    void shutdown();

    bool shutdown_flag = false;
    ::pystd2026::ConditionVariable cv;
    mutable ::pystd2026::Mutex m;

    ::pystd2026::Vector<::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface>> tasks;
    ::pystd2026::Vector<::pystd2026::unique_ptr<::pystd2026::Thread>> threads;

    friend class ::pystd2026::ExecutorWorkerThread;
};

} // namespace pystd2026
