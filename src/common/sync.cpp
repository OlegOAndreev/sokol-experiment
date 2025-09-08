#include "sync.h"

#include <cstddef>
#include <cstdint>
#include <mutex>

#include "slog.h"
#include "thread.h"

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
            std::lock_guard<std::mutex> lock{mutex};
        }
        condvar.notify_all();
    } else if (result <= 0) {
        abort();
    }
}

bool TaskLatch::done() const {
    return count == 0;
}

void TaskLatch::wait() {
    if (local_thread_pool() != nullptr) {
        SLOG_ERROR("TaskLatch::wait() called inside thread pool");
    }
    std::unique_lock<std::mutex> lock{mutex};
    condvar.wait(lock, [this] { return count == 0; });
}
