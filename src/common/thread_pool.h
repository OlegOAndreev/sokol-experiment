#pragma once

#include <functional>
#include <future>
#include <memory>
#include <utility>

#include "struct.h"

// Basic thread pool with single shared task queue. Tasks must accept no parameters and return nothing.
class ThreadPool {
public:
    // Initialize thread pool with default hardware concurrency.
    ThreadPool();
    // Initialize thread pool with given number of threads.
    ThreadPool(size_t num_threads);
    // Shutdown and destroy thread pool.
    ~ThreadPool();

    DISABLE_MOVE_AND_COPY(ThreadPool);

    // Shutdown thread pool: do not accept more tasks, wait until all the current tasks are completed.
    void shutdown();

    // Submit new task. The task must be a callable of type void()
    template <typename F>
    bool submit(F&& task) {
        return submit_impl(std::forward<F>(task));
    }

    // Return number of threads in the pool.
    size_t num_threads() const;

    // Returns true if the caller is in worker thread.
    static bool is_worker_thread();

private:
    using Task = std::function<void()>;

    struct Impl;
    std::unique_ptr<Impl> impl;

    bool submit_impl(Task&& task);
};

// Submit task which returns a value and return a future for that value. Task must be callable of type SomeValue().
template <typename F>
auto submit_future(ThreadPool& pool, F&& f) -> std::future<decltype(f())> {
    std::promise<decltype(f())> promise;
    std::future<decltype(f())> future = promise.get_future();
    pool.submit([promise = std::move(promise), f = std::forward<F>(f)]() mutable { promise.set_value(f()); });
    return promise.get_future();
}

// Global thread pool with default concurrency, use it for CPU bound tasks.
extern ThreadPool global_thread_pool;
