#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "struct.h"


// Simple multithreaded queue which supports closing.
template <typename T>
class MTQueue {
public:
    // Close queue, no more items will be added, but older items will be popped.
    void close() {
        std::lock_guard<std::mutex> lock{mutex};
        closed = true;
        condvar.notify_all();
    }

    // Push item in queue if it is not closed, or return false if closed.
    template <typename U>
    bool push(U&& value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (closed) {
            return false;
        }
        deque.emplace_back(std::forward<U>(value));
        condvar.notify_one();
        return true;
    }

    // Pop item from queue, wait for item if queue is empty and return false if queue is empty and closed.
    bool pop(T& value) {
        std::unique_lock<std::mutex> lock{mutex};
        condvar.wait(lock, [this] { return closed || !deque.empty(); });
        if (deque.empty()) {
            return false;
        }
        value = std::move(deque.front());
        deque.pop_front();
        return true;
    }

    // Pop item from queue if queue is not empty, return false if queue is empty.
    bool poll(T& value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (deque.empty()) {
            return false;
        }
        value = std::move(deque.front());
        deque.pop_front();
        return true;
    }

    // Return queue size (should it be optimized with relaxed atomics?).
    size_t size() const {
        std::lock_guard<std::mutex> lock{mutex};
        return deque.size();
    }

private:
    std::deque<T> deque;
    bool closed = false;

    mutable std::mutex mutex;
    std::condition_variable condvar;
};


// A future, which supports polling its state, unlike std::future, and has builtin counter but does not store value.
class CountingFuture {
public:
    // Initialize the future with desired count.
    CountingFuture(size_t count) : state(std::make_shared<State>()) {
        state->store(count);
    }

    // Decrement the count by one, waking the waiting threads if the count becomes zero.
    void decrement() {
        if (state->fetch_sub(1) == 1) {
            state->notify_all();
        }
    }

    // Wait until the count is zero.
    void wait() {
        while (true) {
            size_t cur = state->load();
            if (cur == 0) {
                break;
            }
            state->wait(cur);
        }
    }

    // Return current remaining count.
    size_t remaining() const {
        return state->load();
    }

    // Return true if future is complete, i.e. count is zero.
    bool done() const {
        return state->load() == 0;
    }

private:
    using State = std::atomic<size_t>;
    std::shared_ptr<State> state;
};


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


// Submit multiple tasks for parallel processing: run callable f for each element in input:
//   for i in 0..n { output[i] = f(input[i]) }
// The future will complete when all elements are processed.
template <typename F, typename I, typename O>
CountingFuture submit_parallel_map(ThreadPool& pool, const I* input, O* output, size_t n, F f) {
    // Split into more tasks than threads to account for uneven workloads.
    size_t per_task = n / (pool.num_threads() * 4);
    if (per_task == 0) {
        return CountingFuture{0};
    }
    // If n is not divisible by per_task, add one more task, otherwise the last task can be unbalanced.
    size_t num_tasks = (n + per_task - 1) / per_task;

    CountingFuture future{num_tasks};
    for (size_t i = 0; i < num_tasks; i++) {
        size_t from_idx = i * per_task;
        size_t to_idx = from_idx + per_task;
        if (i == num_tasks - 1) {
            to_idx = n;
        }
        pool.submit([=]() mutable {
            for (size_t idx = from_idx; idx < to_idx; idx++) {
                output[idx] = f(input[idx]);
            }
            future.decrement();
        });
    }

    return future;
}

// A wrapper for submit_parallel_map() which accepts containers such as std::vector instead of raw pointers and
// automatically resizes the output container. Containers must have methods size(), resize(size_t) and data().
template <typename F, typename C1, typename C2>
CountingFuture submit_parallel_map(ThreadPool& pool, const C1& input, C2& output, F f) {
    size_t n = input.size();
    output.resize(n);
    return submit_parallel_map(pool, input.data(), output.data(), n, f);
}

// Global thread pool with default concurrency, use it for CPU bound tasks.
extern ThreadPool global_thread_pool;
