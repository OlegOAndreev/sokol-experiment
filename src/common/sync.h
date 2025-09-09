#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <forward_list>
#include <mutex>

#include "queue.h"


#define CHECK_THREAD_POOL_BLOCKING

#if defined(CHECK_THREAD_POOL_BLOCKING)
#include "slog.h"
#include "thread_name.h"
#endif


// A simple replacement for std::latch.
class TaskLatch {
public:
    // Init TaskLatch with required count, count can't be greater than PTRDIFF_MAX.
    TaskLatch(size_t count);
    // Decrement the counter by given amount, signal the waiters if counter reaches zero.
    void count_down(size_t amount = 1);
    // Return true if counter is zero.
    bool done() const;
    // Wait until counter reaches zero. Cannot be called from ThreadPool tasks.
    void wait();

private:
    std::atomic<ptrdiff_t> count;
    std::mutex mutex;
    std::condition_variable condvar;
};


// Multi-producer multi-consumer unbounded lock-based queue which blocks on empty. The queue can be closed: no new
// elements can be pushed after closing and pop() will return false if the queue is empty.
template <typename T>
class MPMCQueue {
public:
    // Initialize the queue with capacity.
    MPMCQueue(size_t capacity = 16) : inner(capacity) {
    }

    // Close the queue.
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            closed = true;
        }
        condvar.notify_all();
    }

    // Push new element if the queue has not been closed, return false if it has been closed.
    template <typename U>
    bool try_push(U&& item) {
        // Call the constructor outside of the lock.
        T value = std::forward<U>(item);
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (closed) {
                return false;
            }
            inner.push(std::move(value));
        }
        queue_size++;
        condvar.notify_one();
        return true;
    }

    // Push new element if the queue has not been closed, abort if it has been closed.
    template <typename U>
    void push(U&& item) {
        if (!try_push(std::forward<U>(item))) {
            abort();
        }
    }

    // Retrieve the first element or return false.
    bool try_pop(T& dst) {
        std::unique_lock<std::mutex> lock(mutex);
        if (inner.empty()) {
            return false;
        }
        dst = std::move(inner.front());
        inner.pop();
        queue_size--;
        return true;
    }

    // Retrieve the first element. Wait until the queue becomes not empty or is closed. Return false if the queue is
    // closed.
    bool pop(T& dst) {
#if defined(CHECK_THREAD_POOL_BLOCKING)
        if (is_thread_pool_worker()) {
            SLOG_ERROR("MPMCQueue::pop() called inside the thread pool");
        }
#endif
        std::unique_lock<std::mutex> lock(mutex);
        while (inner.empty()) {
            if (closed) {
                return false;
            }
            condvar.wait(lock);
        }
        dst = std::move(inner.front());
        inner.pop();
        queue_size--;
        return true;
    }

    // Return the queue size.
    size_t size() const {
        return queue_size;
    }

    // Return true if the queue is empty.
    bool empty() const {
        return queue_size == 0;
    }

private:
    std::mutex mutex;
    std::condition_variable condvar;
    Queue<T> inner;
    bool closed = false;
    std::atomic<size_t> queue_size = 0;
};
