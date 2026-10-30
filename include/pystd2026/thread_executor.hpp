// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/threading.hpp>

namespace pystd2026 {

class ExecutorWorkerThread;

struct FutureData {
    virtual ~FutureData() = default;
    ::pystd2026::Mutex m;
    ::pystd2026::ConditionVariable cv;
    int refcount = 2;
    bool has_value = false;
    // a bool that the value has been given out maybe?

    virtual void *get_value() noexcept { return nullptr; }
    virtual void set_value(void *data) noexcept {}
};

struct FuturePair;

template<typename T> class Future2;

class Future {
public:
    Future() noexcept = default;
    Future(Future &&o) noexcept {
        data = o.data;
        o.data = nullptr;
    }

    ~Future() {
        if(data) {
            get();
        }
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

    void *get() {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(data->m);
        while(!data->has_value) {
            data->cv.wait(data->m);
        }
        return data->get_value();
    }

    static FuturePair create_future_pair();

    Future(FutureData *fd) : data{fd} {}

private:
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

    void set(void *ctx = nullptr) {
        {
            ::pystd2026::LockGuard<::pystd2026::Mutex> lg(data->m);
            if(data->has_value) {
                throw ::pystd2026::PyException("Tried to set value in a promise twice.");
            }
            data->set_value(ctx);
            data->has_value = true;
            if(data->refcount > 1) {
                data->cv.signal();
            }
        }
        release_data();
    }

    Promise(FutureData *fd) noexcept : data{fd} {}

private:
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

template<typename T> struct FutureData2 : public FutureData {
    ~FutureData2() = default;

    void *get_value() noexcept override { return &stored_data; }
    void set_value(void *data) noexcept override {
        stored_data = ::pystd2026::move(*reinterpret_cast<T *>(data));
    }

    T stored_data;
};

template<> struct FutureData2<void> : public FutureData {
    ~FutureData2() = default;
    void *get_value() noexcept override { return nullptr; }
    void set_value(void *data) noexcept override {};
};

template<typename T> struct FuturePair2;

template<typename T> struct ref_or_void {
    typedef T &type;
};

template<> struct ref_or_void<void> {
    typedef void type;
};

template<typename T> using ref_or_void_t = typename ref_or_void<T>::type;

template<typename T> class Future2 {
public:
    Future2() noexcept = default;
    explicit Future2(::pystd2026::Future2<T> &&of) noexcept
        : underlying{::pystd2026::move(of.underlying)} {
        of.underlying = nullptr;
    }
    // Future2(::pystd2026::Future2<T> &&) noexcept = default;

    Future2 &operator=(::pystd2026::Future2<T> &&) noexcept = default;

    ref_or_void_t<T> get() {
        if constexpr(::pystd2026::is_same_v<void, T>) {
            underlying.get();
        } else {
            static_assert(::pystd2026::is_reference_v<ref_or_void_t<T>>);
            return *reinterpret_cast<T *>(underlying.get());
        }
    }

    static FuturePair2<T> create_future_pair2();

private:
    Future2(FutureData2<T> *fd) : underlying{fd} {}

    ::pystd2026::Future underlying;
};

template<typename T> class Promise2 {
public:
    Promise2() noexcept = default;
    explicit Promise2(::pystd2026::Promise &&of) noexcept : underlying{::pystd2026::move(of)} {}
    Promise2(::pystd2026::Promise2<T> &&) noexcept = default;

    Promise2 &operator=(::pystd2026::Promise2<T> &&) noexcept = default;

    void set(T &result) { underlying.set(static_cast<void *>(&result)); }

    Promise2(FutureData *fd) noexcept : underlying{fd} {}

private:
    ::pystd2026::Promise underlying;
};

template<> class Promise2<void> {
public:
    Promise2() noexcept = default;
    explicit Promise2(::pystd2026::Promise &&of) noexcept : underlying{::pystd2026::move(of)} {}
    Promise2(::pystd2026::Promise2<void> &&) noexcept = default;

    Promise2 &operator=(::pystd2026::Promise2<void> &&) noexcept = default;

    void set() { underlying.set(); }

    Promise2(FutureData *fd) noexcept : underlying{fd} {}

private:
    ::pystd2026::Promise underlying;
};

template<typename T> struct FuturePair2 {
    ::pystd2026::Future2<T> f;
    ::pystd2026::Promise2<T> p;
};

template<typename T> inline FuturePair2<T> Future2<T>::create_future_pair2() {
    auto pair_data = new FutureData2<T>{};

    return FuturePair2<T>{pair_data, pair_data};
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
