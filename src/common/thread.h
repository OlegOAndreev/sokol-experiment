#pragma once

#include <functional>
#include <memory>

#include "struct.h"


// Basic thread pool with single shared task queue. Tasks must accept no parameters and return nothing.
class ThreadPool {
public:
    // Initialize thread pool with given name and number of threads.
    ThreadPool(const char* name, size_t num_threads);
    // Shutdown and destroy thread pool.
    ~ThreadPool();

    // Shutdown thread pool: do not accept more tasks, wait until all the current tasks are completed.
    void shutdown();

    // Submit new task to execute in pool. The f must be a callable of type void f()
    template <typename F>
    bool submit(F&& f) {
        return submit_impl(Task{std::forward<F>(f), nullptr, 0, 0});
    }

    // Submit a range of tasks to execute in pool. The f must be a callable of type void f(size_t index).
    template <typename F>
    bool submit_range(F&& f, size_t n) {
        if (n == 0) {
            return true;
        }
        return submit_impl(Task{nullptr, std::forward<F>(f), 0, n});
    }

    // Return number of threads in the pool.
    size_t num_threads() const;

    // Return the pool name.
    const char* name() const;

private:
    DISABLE_MOVE_AND_COPY(ThreadPool);

    struct Impl;
    std::unique_ptr<Impl> impl;

    struct Task {
        // This could've been a union, but making unions in C++ sucks and std::function is 32 bytes anyway.
        std::function<void()> func_single;
        std::function<void(size_t)> func_range;
        // The start is updated by workers grabbing a piece of task.
        size_t start = 0;
        // If end == 0, run func_single, otherwise run func_range.
        size_t end = 0;
    };
    bool submit_impl(Task&& task);
};

// Return current thread pool: global thread pool if called outside of a ThreadPool task, or the ThreadPool executing
// current task.
ThreadPool& thread_pool();

// Return the global thread pool with default concurrency, should be the default choice for CPU bound tasks.
ThreadPool& global_thread_pool();

// Return the local thread pool if called inside a ThreadPool task, nullptr if called outside of a ThreadPool task.
ThreadPool* local_thread_pool();
