#include "sync.h"

#include <cstddef>
#include <cstdint>
#include <mutex>


TaskLatch::TaskLatch(size_t count) : count(ptrdiff_t(count)) {
    if (count > PTRDIFF_MAX) {
        abort();
    }
}

void TaskLatch::count_down(size_t amount) {
    if (amount > PTRDIFF_MAX) {
        abort();
    }
    ptrdiff_t result = count.fetch_sub(ptrdiff_t(amount));
    if (result == ptrdiff_t(amount)) {
        {
            // An empty lock is required to prevent missed notify when the waiting thread calls wait() concurrently with
            // count_down().
            std::lock_guard<std::mutex> lock(mutex);
        }
        condvar.notify_all();
    } else if (result <= 0) {
        abort();
    }
}

void TaskLatch::reset(size_t count) {
    if (count > PTRDIFF_MAX) {
        abort();
    }
    this->count.store(ptrdiff_t(count));
    if (count == 0) {
        {
            std::lock_guard<std::mutex> lock(mutex);
        }
        condvar.notify_all();
    }
}

size_t TaskLatch::remaining() const {
    return count.load();
}

bool TaskLatch::done() const {
    return count.load() == 0;
}

void TaskLatch::wait() {
#if defined(CHECK_THREAD_POOL_BLOCKING)
    if (is_thread_pool_worker()) {
        SLOG_ERROR("TaskLatch::wait() called inside the thread pool");
    }
#endif
    std::unique_lock<std::mutex> lock(mutex);
    condvar.wait(lock, [this] { return count.load() == 0; });
}
