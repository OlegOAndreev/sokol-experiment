#pragma once

#include <algorithm>
#include <memory>
#include <utility>

// Simplest ringbuffer-based queue. Not thread-safe. Assumes that items are default-constructible and default
// constructor is cheap. The ring buffer is always the power of two to optimize operations with head/tail.
template <typename T>
class Queue {
public:
    // Initialize the queue with capacity.
    Queue(size_t capacity = 16) : data(std::make_unique<T[]>(capacity)), capacity(capacity) {
        if ((capacity & (capacity - 1)) != 0) {
            // Capacity must be power of two.
            abort();
        }
    }

    // Push new element and resize the buffer if required.
    template <typename U>
    void push(U&& item) {
        if (tail - head == capacity) {
            ensure_capacity();
        }
        data[tail & (capacity - 1)] = std::forward<U>(item);
        // See size() why this increment is correct.
        tail++;
    }

    // Retrieve the first element.
    T& front() {
        if (head == tail) {
            // The queue is empty.
            abort();
        }
        return data[head & (capacity - 1)];
    }

    // Pop the first element.
    void pop() {
        if (head == tail) {
            // The queue is empty.
            abort();
        }
        // See size() why this increment is correct.
        head++;
    }

    // Return the queue size.
    size_t size() const {
        // NOTE: Queue requires capacity to be power of two not only for faster modulo operations (& instead of %), but
        // also because it relies on wrapping around SIZE_MAX for head and tail:
        //  1. size() == tail - head stands even if tail < head
        //  2. (tail + 1) & (capacity - 1) is the next item after tail & (capacity - 1) even when tail wraps around
        return tail - head;
    }

    // Return true if the queue is empty.
    bool empty() const {
        return head == tail;
    }

private:
    // Assume T default construction is fast: use std::unique_ptr<T[]> instead of std::vector<T>
    std::unique_ptr<T[]> data;
    size_t capacity;
    size_t head = 0;
    size_t tail = 0;

    void ensure_capacity() {
        size_t new_capacity = capacity * 2;
        std::unique_ptr<T[]> new_data = std::make_unique<T[]>(new_capacity);

        size_t cur_size = tail - head;
        size_t head_idx = head & (capacity - 1);
        size_t tail_idx = tail & (capacity - 1);
        if (head_idx < tail_idx) {
            std::move(data.get() + head_idx, data.get() + tail_idx, new_data.get());
        } else {
            std::move(data.get() + head_idx, data.get() + capacity, new_data.get());
            std::move(data.get(), data.get() + tail_idx, new_data.get() + (capacity - head_idx));
        }

        data = std::move(new_data);
        capacity = new_capacity;
        head = 0;
        tail = cur_size;
    }
};
