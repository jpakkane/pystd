// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/thread_executor.hpp>
#include <unistd.h>

namespace pystd2026 {

class ExecutorWorkerThread : public ::pystd2026::ThreadInterface {
public:
    ExecutorWorkerThread(ThreadExecutor *executor) : ex{executor} {}

    void *run() override {
        //        printf("Starting worker thread.\n");
        ::pystd2026::Optional<TaskData> active_task;
        while(true) {
            //          printf("Grabbing task.\n");
            {
                ::pystd2026::LockGuard<::pystd2026::Mutex> lg(ex->m);
                active_task = ex->tasks.pop_front();
                if(!active_task) {
                    if(ex->shutdown_flag) {
                        break;
                    }
                }
            }
            if(active_task) {
                auto &atask = active_task.value();
                atask.task->run(atask.p);
                active_task.clear();
            }
            {
                ::pystd2026::LockGuard<::pystd2026::Mutex> lg(ex->m);
                if(ex->tasks.is_empty()) {
                    if(ex->shutdown_flag) {
                        break;
                    }
                    ex->cv.wait(ex->m);
                }
            }
        }
        //    printf("Exiting worker.\n");
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
        auto ti =
            ::pystd2026::unique_ptr<::pystd2026::ThreadInterface>(new ExecutorWorkerThread(this));
        auto t = ::pystd2026::unique_ptr<::pystd2026::Thread>(new Thread(pystd2026::move(ti)));
        threads.push_back(::pystd2026::move(t));
    }
}

ThreadExecutor::~ThreadExecutor() { shutdown(); }

void ThreadExecutor::shutdown() {
    {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(m);
        shutdown_flag = true;
    }
    cv.broadcast();
}

void ThreadExecutor::reserve(size_t num_tasks) noexcept {
    {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(m);
        tasks.reserve(num_tasks);
    }
}

size_t ThreadExecutor::size() const noexcept {
    {
        ::pystd2026::LockGuard<::pystd2026::Mutex> lg(m);
        return tasks.size();
    }
}

} // namespace pystd2026
