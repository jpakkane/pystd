// SPDX-License-Identifier: Apache-2.0 + LLVM exception
// Copyright 2025 Jussi Pakkanen

#include <pystd2026/threading.hpp>
#include <errno.h>

namespace pystd2026 {

static void *ThreadInvoker(void *iface_c) {
    auto iface = reinterpret_cast<::pystd2026::ThreadInterface *>(iface_c);
    return iface->run();
}

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
        throw_errno_error();
    }
}

bool Mutex::try_lock() {
    auto rc = pthread_mutex_trylock(&m);
    if(rc == EINVAL) {
        throw_errno_error();
    }
    return rc == 0;
}

void Mutex::unlock() {
    auto rc = pthread_mutex_unlock(&m);
    if(rc != 0) {
        throw_errno_error();
    }
}

Thread::Thread(::pystd2026::unique_ptr<ThreadInterface> state_in)
    : thread_state(::pystd2026::move(state_in)) {
    pthread_attr_t attr;
    auto rc = pthread_attr_init(&attr);
    if(rc != 0) {
        throw_errno_error();
    }
    rc = pthread_create(&thread, &attr, ThreadInvoker, thread_state.get());
    pthread_attr_destroy(&attr);
    if(rc != 0) {
        throw_errno_error();
    }
}

Thread::~Thread() {
    if(!thread_is_finalized) {
        join();
    }
}

void *Thread::join() {
    void *retval;
    pthread_join(thread, &retval);
    thread_is_finalized = true;
    return retval;
}

void Thread::detach() {
    pthread_detach(thread);
    thread_is_finalized = true;
}

ConditionVariable::ConditionVariable() noexcept {
    if(pthread_cond_init(&cond, nullptr) != 0) {
        throw_errno_error();
    }
}

ConditionVariable::~ConditionVariable() {
    if(pthread_cond_destroy(&cond) != 0) {
        fprintf(stderr, "Could not destroy ConditionVariable: %s\n", strerror(errno));
    }
}

void ConditionVariable::signal() {
    if(pthread_cond_signal(&cond) != 0) {
        throw_errno_error();
    }
}

void ConditionVariable::broadcast() {
    if(pthread_cond_broadcast(&cond) != 0) {
        throw_errno_error();
    }
}

void ConditionVariable::wait(::pystd2026::Mutex &m) {
    if(pthread_cond_wait(&cond, &m.native()) != 0) {
        throw_errno_error();
    }
}

} // namespace pystd2026
