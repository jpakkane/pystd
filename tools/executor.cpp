// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2026 Jussi Pakkanen

#include <pystd2026/thread_executor.hpp>

#include <time.h>

class PrintTask : public ::pystd2026::ExecutorTaskInterface {
public:
    explicit PrintTask(const char *msg) : str(msg) {}

    void run() override {
        const int num_times = 4;
        timespec dur;
        dur.tv_sec = 2;
        dur.tv_nsec = 0;
        for(int i = 0; i < num_times; ++i) {
            nanosleep(&dur, nullptr);
            printf("%s\n", str.c_str());
        }
    }

private:
    ::pystd2026::CString str;
};

int main() {
    {
        int num_threads = 4;
        int num_tasks = 10;
        char buf[20];
        pystd2026::ThreadExecutor ex1(num_threads);
        for(int task = 0; task < num_tasks; ++task) {
            snprintf(buf, 20, "task: %d", task);
            pystd2026::unique_ptr<pystd2026::ExecutorTaskInterface> up(new PrintTask(buf));
            auto f = ex1.submit<void>(pystd2026::move(up));
        }
        timespec dur;
        dur.tv_sec = 2;
        dur.tv_nsec = 0;
        nanosleep(&dur, nullptr);
    }
    return 0;
}
