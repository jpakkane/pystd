// SPDX-License-Identifier: Apache-2.0
// Copyright 2025 Jussi Pakkanen

#include <pystd2025_threading.hpp>
#include <errno.h>

namespace pystd2025 {

Mutex::Mutex() noexcept {
    m = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&m, nullptr);
}

Mutex::~Mutex() {
    if(pthread_mutex_destroy(&m) != 0) {
        fprintf(stderr, "Tried to destroy a locked mutex.\n");
    }
}

void Mutex::lock() {
    auto rc = pthread_mutex_lock(&m);
    if(rc != 0) {
        throw PyException(strerror(rc));
    }
}

bool Mutex::try_lock() {
    auto rc = pthread_mutex_trylock(&m);
    if(rc == EINVAL) {
        throw PyException(strerror(rc));
    }
    return rc == 0;
}

void Mutex::unlock() {
    auto rc = pthread_mutex_unlock(&m);
    if(rc != 0) {
        throw PyException(strerror(rc));
    }
}

Thread::Thread(void *(*thread_main_func)(void *), void *ctx) {
    pthread_attr_t attr;
    auto rc = pthread_attr_init(&attr);
    if(rc != 0) {
        throw PyException(strerror(rc));
    }
    rc = pthread_create(&thread, &attr, thread_main_func, ctx);
    pthread_attr_destroy(&attr);
    if(rc != 0) {
        throw PyException(strerror(rc));
    }
}

Thread::~Thread() {
    if(!thread_is_finalized) {
        join();
    }
}

void Thread::join() {
    pthread_join(thread, nullptr);
    thread_is_finalized = true;
}

void Thread::detach() {
    pthread_detach(thread);
    thread_is_finalized = true;
}

} // namespace pystd2025
