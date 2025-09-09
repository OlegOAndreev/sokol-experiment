#include "thread.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "queue.h"
#include "slog.h"
#include "thread_name.h"


thread_local ThreadPool* tl_worker_pool = nullptr;
thread_local size_t tl_worker_idx = 0;

struct ThreadPool::Impl {
    // We do not want to split the ranged tasks into too many small pieces because this increases queue contention.
    // This ratio accounts for the imbalance of subtasks inside one ranged tasks.
    static const size_t range_split_ratio = 4;

    std::string name;
    std::vector<std::thread> workers;

    std::mutex mutex;
    std::condition_variable condvar;
    Queue<Task> queue;
    bool closed = false;

    std::atomic<size_t> num_inflight_tasks = 0;

    void init(const char* name, size_t num_threads, ThreadPool* parent) {
        this->name = name;
        for (size_t idx = 0; idx < num_threads; idx++) {
            workers.emplace_back([this, parent, idx] {
                tl_worker_pool = parent;
                tl_worker_idx = idx;
                run_worker();
            });
        }
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (closed) {
                return;
            }
            closed = true;
        }
        condvar.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    bool submit(Task&& task) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (closed) {
                return false;
            }
            queue.push(std::move(task));
        }
        num_inflight_tasks.fetch_add(1);
        // We rely on the worker to notify next thread if required.
        condvar.notify_one();
        return true;
    }

    void run_worker() {
        while (true) {
            Task task;
            bool notify_next = false;
            bool popped_task = false;
            {
                std::unique_lock<std::mutex> lock(mutex);
                while (queue.empty()) {
                    if (closed) {
                        return;
                    }
                    condvar.wait(lock);
                }
                Task& front = queue.front();
                task = split_task(front);
                if (front.end == front.start) {
                    queue.pop();
                    popped_task = true;
                }
                if (!queue.empty()) {
                    // There are more tasks in the queue, wake the next worker before running our part of the task.
                    notify_next = true;
                }
            }
            if (notify_next) {
                condvar.notify_one();
            }

            run_task(task);
            if (popped_task) {
                num_inflight_tasks.fetch_sub(1);
            }
        }
    }

    // Split the next part of the task.
    Task split_task(Task& from) {
        if (from.end > 0) {
            size_t per_worker = from.end / (workers.size() * range_split_ratio);
            if (per_worker == 0) {
                per_worker = 1;
            }
            size_t next_start = from.start + per_worker;
            if (next_start > from.end) {
                next_start = from.end;
            }
            Task result = from;
            result.end = next_start;
            from.start = next_start;
            return result;
        } else {
            Task result = std::move(from);
            from.start = from.end;
            return result;
        }
    }

    void run_task(const Task& task) {
        if (task.end > 0) {
            for (size_t i = task.start; i < task.end; i++) {
                task.func_range(i);
            }
        } else {
            task.func_single();
        }
    }
};


ThreadPool::ThreadPool(const char* name, size_t num_threads) : impl(new Impl()) {
    impl->init(name, num_threads, this);
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    impl->shutdown();
}

size_t ThreadPool::num_inflight_tasks() {
    return impl->num_inflight_tasks.load();
}

size_t ThreadPool::num_threads() const {
    return impl->workers.size();
}

const char* ThreadPool::name() const {
    return impl->name.c_str();
}

bool ThreadPool::submit_impl(Task&& task) {
    return impl->submit(std::move(task));
}

ThreadPool& thread_pool() {
    if (tl_worker_pool != nullptr) {
        return *tl_worker_pool;
    }
    return global_thread_pool();
}

ThreadPool& global_thread_pool() {
    // Leave one CPU core for main+audio thread.
    size_t num_threads = std::thread::hardware_concurrency() - 1;
    if (num_threads == 0) {
        num_threads = 1;
    }
    static ThreadPool thread_pool("global-pool", num_threads);
    return thread_pool;
}

ThreadPool* local_thread_pool() {
    return tl_worker_pool;
}

const char* local_thread_pool_name() {
    if (tl_worker_pool != nullptr) {
        return tl_worker_pool->name();
    }
    return nullptr;
}

size_t local_thread_pool_worker_id() {
    if (tl_worker_pool == nullptr) {
        return 0;
    }
    return tl_worker_idx + 1;
}

bool is_thread_pool_worker() {
    return tl_worker_pool != nullptr;
}
