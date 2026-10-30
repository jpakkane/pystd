// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#pragma once

#include <pystd2026/threading.hpp>

namespace pystd2026 {

class ExecutorWorkerThread;

struct FutureDataBase {
    virtual ~FutureDataBase() = default;
    ::pystd2026::Mutex m;
    ::pystd2026::ConditionVariable cv;
    int refcount = 2;
    bool has_value = false;
    // a bool that the value has been given out maybe?

    virtual void *get_value() noexcept { return nullptr; }
    virtual void set_value(void *data) noexcept {}
};

template<typename T> struct FuturePair;

template<typename T> class Future;

class TypeErasedFuture {
public:
    TypeErasedFuture() noexcept = default;
    TypeErasedFuture(TypeErasedFuture &&o) noexcept {
        data = o.data;
        o.data = nullptr;
    }

    ~TypeErasedFuture() {
        if(data) {
            get();
        }
        release_data();
    }

    TypeErasedFuture &operator=(TypeErasedFuture &&o) noexcept {
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

    TypeErasedFuture(FutureDataBase *fd) : data{fd} {}

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

    FutureDataBase *data = nullptr;
};

class TypeErasedPromise {
public:
    friend class TypeErasedFuture;
    TypeErasedPromise() noexcept = default;
    TypeErasedPromise(TypeErasedPromise &&o) noexcept {
        data = o.data;
        o.data = nullptr;
    }

    TypeErasedPromise &operator=(TypeErasedPromise &&o) noexcept {
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

    TypeErasedPromise(FutureDataBase *fd) noexcept : data{fd} {}

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

    FutureDataBase *data = nullptr;
};

template<typename T> struct FutureData : public FutureDataBase {
    ~FutureData() = default;

    void *get_value() noexcept override { return &stored_data; }
    void set_value(void *data) noexcept override {
        stored_data = ::pystd2026::move(*reinterpret_cast<T *>(data));
    }

    T stored_data;
};

template<> struct FutureData<void> : public FutureDataBase {
    ~FutureData() = default;
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

template<typename T> class Future {
public:
    Future() noexcept = default;
    Future(::pystd2026::Future<T> &&of) noexcept = default;
    // Future(::pystd2026::Future<T> &&) noexcept = default;

    Future &operator=(::pystd2026::Future<T> &&) noexcept = default;

    ref_or_void_t<T> get() {
        if constexpr(::pystd2026::is_same_v<void, T>) {
            underlying.get();
        } else {
            static_assert(::pystd2026::is_reference_v<ref_or_void_t<T>>);
            return *reinterpret_cast<T *>(underlying.get());
        }
    }

    static FuturePair<T> create_future_pair();

private:
    Future(FutureData<T> *fd) : underlying{fd} {}

    ::pystd2026::TypeErasedFuture underlying;
};

template<typename T> class Promise {
public:
    Promise() noexcept = default;

    explicit Promise(::pystd2026::TypeErasedPromise &&of) noexcept
        : underlying{::pystd2026::move(of)} {}
    Promise(::pystd2026::Promise<T> &&) noexcept = default;

    Promise &operator=(::pystd2026::Promise<T> &&) noexcept = default;

    void set(T &result) { underlying.set(static_cast<void *>(&result)); }

    Promise(FutureDataBase *fd) noexcept : underlying{fd} {}

    ::pystd2026::TypeErasedPromise swipe() { return ::pystd2026::move(underlying); }

private:
    ::pystd2026::TypeErasedPromise underlying;
};

template<> class Promise<void> {
public:
    Promise() noexcept = default;
    explicit Promise(::pystd2026::TypeErasedPromise &&of) noexcept
        : underlying{::pystd2026::move(of)} {}
    Promise(::pystd2026::Promise<void> &&) noexcept = default;

    Promise &operator=(::pystd2026::Promise<void> &&) noexcept = default;

    void set() { underlying.set(); }

    Promise(FutureDataBase *fd) noexcept : underlying{fd} {}

    ::pystd2026::TypeErasedPromise swipe() { return ::pystd2026::move(underlying); }

private:
    ::pystd2026::TypeErasedPromise underlying;
};

template<typename T> struct FuturePair {
    ::pystd2026::Future<T> f;
    ::pystd2026::Promise<T> p;
};

template<typename T> inline FuturePair<T> Future<T>::create_future_pair() {
    auto pair_data = new FutureData<T>{};

    return FuturePair<T>{pair_data, pair_data};
}

class ExecutorTaskInterface {
public:
    friend class TaskExecutor;
    virtual ~ExecutorTaskInterface() = default;

    virtual void run(::pystd2026::TypeErasedPromise &) noexcept = 0;

private:
    ::pystd2026::TypeErasedPromise promise;
};

struct TaskData {
    ::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface> task;
    ::pystd2026::TypeErasedPromise p;
};

class ThreadExecutor final {
public:
    explicit ThreadExecutor(int num_threads = -1);
    ~ThreadExecutor();

    ThreadExecutor(const ThreadExecutor &) = delete;
    ThreadExecutor &operator=(const ThreadExecutor &) = delete;

    template<typename T>
    ::pystd2026::Future<T>
    submit(::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface> task) {
        FuturePair<T> fp;
        {
            ::pystd2026::LockGuard<::pystd2026::Mutex> lg(m);
            if(shutdown_flag) {
                throw ::pystd2026::PyException(
                    "Fatal: trying to submit work to an executor that is shutting down.");
            }
            fp = ::pystd2026::Future<T>::create_future_pair();
            tasks.emplace_back(::pystd2026::move(task), ::pystd2026::move(fp.p.swipe()));
        }
        cv.signal();
        return ::pystd2026::move(fp.f);
    }
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
