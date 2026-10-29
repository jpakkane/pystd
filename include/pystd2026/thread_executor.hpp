// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/threading.hpp>

namespace pystd2026 {

class ExecutorWorkerThread;

struct FutureData {
    // virtual ~FutureData();
    ::pystd2026::Mutex m;
    ::pystd2026::ConditionVariable cv;
    int refcount = 2;
    bool has_value = false;
    // a bool that the value has been given out maybe?

    // virtual void* get_value() noexcept { return nullptr; }
};

struct FuturePair;

class Future {
public:
    Future() noexcept = default;
    Future(Future &&o) noexcept {
        data = o.data;
        o.data = nullptr;
    }

    ~Future() {
        get();
        release_data();
    }

    Future &operator=(Future &&o) noexcept {
        if(this != &o) {
            release_data();
            data = o.data;
            o.data = nullptr;
        }
        return *this;
    }

    void get() {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(data->m);
        while(!data->has_value) {
            data->cv.wait(data->m);
        }
    }

    static FuturePair create_future_pair();

private:
    Future(FutureData *fd) : data{fd} {}

    void release_data() noexcept {
        if(data == nullptr) {
            return;
        }
        bool deallocate_it;
        {
            ::pystd2026::LockGuard<::pystd2026::Mutex> lg(data->m);
            --data->refcount;
            deallocate_it = data->refcount == 0;
        }
        if(deallocate_it) {
            delete data;
        }
        data = nullptr;
    }

    FutureData *data = nullptr;
};

class Promise {
public:
    friend class Future;
    Promise() noexcept = default;
    Promise(Promise &&o) noexcept {
        data = o.data;
        o.data = nullptr;
    }

    Promise &operator=(Promise &&o) noexcept {
        if(this != &o) {
            release_data();
            data = o.data;
            o.data = nullptr;
        }
        return *this;
    }

    void set() {
        {
            ::pystd2026::LockGuard<::pystd2026::Mutex> lg(data->m);
            if(data->has_value) {
                throw ::pystd2026::PyException("Tried to set value in a promise twice.");
            }
            data->has_value = true;
            if(data->refcount > 1) {
                data->cv.signal();
            }
        }
        release_data();
    }

private:
    Promise(FutureData *fd) noexcept : data{fd} {}

    void release_data() noexcept {
        if(data == nullptr) {
            return;
        }
        bool deallocate_it;
        {
            ::pystd2026::LockGuard<::pystd2026::Mutex> lg(data->m);
            --data->refcount;
            deallocate_it = data->refcount == 0;
        }
        if(deallocate_it) {
            delete data;
        }
        data = nullptr;
    }

    FutureData *data = nullptr;
};

struct FuturePair {
    Future f;
    Promise p;
};

inline FuturePair Future::create_future_pair() {
    auto pair_data = new FutureData{};
    return FuturePair{pair_data, pair_data};
}

class ExecutorTaskInterface {
public:
    virtual ~ExecutorTaskInterface() = default;

    virtual void run() = 0;

    ::pystd2026::Promise promise;
};

struct TaskData {
    ::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface> task;
    ::pystd2026::Promise p;
};

class ThreadExecutor final {
public:
    explicit ThreadExecutor(int num_threads = -1);
    ~ThreadExecutor();

    ThreadExecutor(const ThreadExecutor &) = delete;
    ThreadExecutor &operator=(const ThreadExecutor &) = delete;

    ::pystd2026::Future submit(::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface> task);

    void reserve(size_t num_tasks) noexcept;

    size_t size() const noexcept;

private:
    void shutdown();

    bool shutdown_flag = false;
    ::pystd2026::ConditionVariable cv;
    mutable ::pystd2026::Mutex m;

    ::pystd2026::Vector<TaskData> tasks;
    ::pystd2026::Vector<::pystd2026::unique_ptr<::pystd2026::Thread>> threads;

    friend class ::pystd2026::ExecutorWorkerThread;
};

} // namespace pystd2026
