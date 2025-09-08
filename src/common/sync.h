#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>

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
