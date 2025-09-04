#pragma once

#include <atomic>
#include <functional>
#include <latch>
#include <memory>

#include "struct.h"


// Basic thread pool with single shared task queue. Tasks must accept no parameters and return nothing.
class ThreadPool {
public:
    // Initialize the thread pool with given name and number of threads. If max_queue_size is 0, the queue is unbounded.
    ThreadPool(const char* name, size_t num_threads, size_t max_queue_size = 0);
    // Shutdown and destroy the thread pool.
    ~ThreadPool();

    // Shutdown the thread pool: do not accept more tasks, wait until all the current tasks are completed.
    void shutdown();

    // Submit a task to execute in the pool. f must be a callable of type `void f()`. Return false if the pool has been
    // shutdown or the task is greater than the 
    template <typename F>
    bool submit(F&& f) {
        Task task{std::forward<F>(f), nullptr, 0, 0, nullptr};
        return submit_impl(&task, 1);
    }

    // Submit a range of tasks to execute in the pool. f must be a callable of type `void f(size_t index)` and will be
    // called with argument in the range 0..n-1 (inclusive). Return false if the pool has been shutdown.
    template <typename F>
    bool submit_for(F&& f, size_t n) {
        if (n == 0) {
            return true;
        }
        Task task{nullptr, std::forward<F>(f), 0, n, nullptr};
        return submit_impl(&task, 1);
    }

    // Submit tasks to execute in the pool and wait for completion. f must be a callable of type `void f()`. Return
    // false if the pool has been shutdown.
    template <typename... F>
    bool parallel(const F&... f) {
        constexpr size_t num_tasks = sizeof...(F);
        Latch latch{num_tasks};
        // Optimize submitting multiple tasks at once by passing an array to sumbit_impl.
        std::array<Task, num_tasks> tasks = {Task{std::cref(f), nullptr, 0, 0, &latch}...};
        if (!submit_impl(tasks.data(), num_tasks)) {
            return false;
        }
        wait_impl(&latch);
        return true;
    }

    // Submit a range of tasks in the pool and wait for completion. f must be a callable of type `void f(size_t index)`
    // and will be called with argument in the range 0..n-1 (inclusive). Return false if the pool has been shutdown.
    template <typename F>
    bool parallel_for(const F& f, size_t n) {
        Latch latch{n};
        Task task{nullptr, std::cref(f), 0, n, &latch};
        // The f is alive until the parallel_for returns, use std::ref to prevent copies.
        if (!submit_impl(&task, 1)) {
            return false;
        }
        wait_impl(&latch);
        return true;
    }

    // Return number of threads in the pool.
    size_t num_threads() const;

    // Return the pool name.
    const char* name() const;

private:
    DISABLE_MOVE_AND_COPY(ThreadPool);

    struct Impl;
    std::unique_ptr<Impl> impl;

    // std::latch/std::counting_semaphore are still pretty buggy both in gcc and clang, let's implement a simple version
    // using the previous primitives.
    struct Latch {
        Latch(size_t n);

        void decrement(size_t count);
        void wait();
    };

    struct Task {
        // This could've been a union, but making unions in C++ sucks and std::function is 32 bytes anyway.
        std::function<void()> func_single;
        std::function<void(size_t)> func_range;
        // The start is updated by workers grabbing a piece of task.
        size_t start = 0;
        // If end == 0, run func_single, otherwise run func_range.
        size_t end = 0;
        // If latch != nullptr, it is signalled when the task completes. The count is 1 for single tasks and complete
        // range for ranged tasks.
        Latch* latch = nullptr;
    };

    // submit_impl moves from tasks
    bool submit_impl(Task* tasks, size_t n);

    // wait_impl tries to run other tasks from the thread pool while we wait for the latch.
    static void wait_impl(Latch* latch);
};

// Return current thread pool: global thread pool if called outside of a ThreadPool task, or the ThreadPool executing
// current task.
ThreadPool& thread_pool();

// Return the global thread pool with default concurrency and default queue size, should be the default choice for CPU
// bound tasks.
ThreadPool& global_thread_pool();

// Return the local thread pool if called inside a ThreadPool task, nullptr if called outside of a ThreadPool task.
ThreadPool* local_thread_pool();
