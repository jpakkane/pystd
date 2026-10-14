// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/thread_executor.hpp>
#include <unistd.h>

namespace pystd2026 {

class ExecutorWorkerThread : public ::pystd2026::ThreadInterface {
public:
    ExecutorWorkerThread(ThreadExecutor *executor) : ex{executor} {}

    void *run() override {
        ::pystd2026::Optional<::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface>>
            active_task;
        while(true) {
            {
                ::pystd2026::LockGuard<::pystd2026::Mutex> lg(ex->m);
                if(ex->shutdown_flag) {
                    break;
                } else {
                    active_task = ex->tasks.pop_front();
                }
            }
            if(active_task) {
                active_task.value()->run();
                active_task.clear();
            }
            {
                ::pystd2026::LockGuard<::pystd2026::Mutex> lg(ex->m);
                if(ex->shutdown_flag) {
                    break;
                }
                ex->cv.wait(ex->m);
            }
        }
        return nullptr;
    }

private:
    ThreadExecutor *ex;
};

ThreadExecutor::ThreadExecutor(int num_threads) {
    if(num_threads <= 0) {
        num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    }
    threads.reserve(num_threads);
    for(int i = 0; i < num_threads; ++i) {
        auto t =
            ::pystd2026::unique_ptr<::pystd2026::ThreadInterface>(new ExecutorWorkerThread(this));
        threads.push_back(::pystd2026::move(t));
    }
}

ThreadExecutor::~ThreadExecutor() { shutdown(); }

void ThreadExecutor::submit(::pystd2026::unique_ptr<::pystd2026::ExecutorTaskInterface> task) {
    {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(m);
        tasks.push_back(::pystd2026::move(task));
    }
    cv.signal();
}

void ThreadExecutor::shutdown() {
    {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(m);
        shutdown_flag = true;
    }
    cv.broadcast();
}

} // namespace pystd2026
