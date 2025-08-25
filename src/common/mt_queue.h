#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>


template <typename T>
class MtQueue {
public:
    void close() {
        std::lock_guard<std::mutex> lock{mutex};
        closed = true;
        condvar.notify_all();
    }

    bool push(const T& value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (closed) {
            return false;
        }
        deque.push_back(value);
        condvar.notify_one();
        return true;
    }

    bool push(T&& value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (closed) {
            return false;
        }
        deque.push_back(value);
        condvar.notify_one();
        return true;
    }

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

    bool poll(T* value) {
        std::lock_guard<std::mutex> lock{mutex};
        if (deque.empty()) {
            return false;
        }
        *value = std::move(deque.front());
        deque.pop_front();
        return true;
    }

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
