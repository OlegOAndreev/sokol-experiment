#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>


// Simple multithreaded queue which supports closing.
template <typename T>
class MtQueue {
public:
    // Close queue, no more items will be added, but older items will be popped.
    void close() {
        std::lock_guard<std::mutex> lock{mutex};
        closed = true;
        condvar.notify_all();
    }

    // Push item in queue if it is not closed, or return false if closed.
    bool push(const T& value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (closed) {
            return false;
        }
        deque.push_back(value);
        condvar.notify_one();
        return true;
    }

    // Push item in queue if it is not closed, or return false if closed.
    bool push(T&& value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (closed) {
            return false;
        }
        deque.push_back(value);
        condvar.notify_one();
        return true;
    }

    // Pop item from queue, wait for item if queue is empty and return false if queue is empty and closed.
    bool pop(T* value) {
        std::unique_lock<std::mutex> lock{mutex};
        condvar.wait(lock, [this] { return closed || !deque.empty(); });
        if (deque.empty()) {
            return false;
        }
        *value = std::move(deque.front());
        deque.pop_front();
        return true;
    }

    // Pop item from queue if queue is not empty, return false if queue is empty.
    bool poll(T* value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (deque.empty()) {
            return false;
        }
        *value = std::move(deque.front());
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
    bool closed;

    mutable std::mutex mutex;
    std::condition_variable condvar;
};
