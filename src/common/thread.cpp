#include "thread.h"

#include <cstddef>
#include <mutex>
#include <semaphore>
#include <string>
#include <thread>
#include <vector>

#include "queue.h"
#include "slog.h"
#include "thread_name.h"


thread_local ThreadPool* tl_worker_pool = nullptr;
thread_local size_t tl_worker_id = 0;

struct ThreadPool::Impl {
    std::string pool_name;
    std::vector<std::thread> workers;

    std::counting_semaphore<> available{0};
    std::mutex mutex;
    Queue<ThreadPool::Task> queue;
    bool closed = false;

    void init(const char* name, size_t num_threads, ThreadPool* parent) {
        pool_name = name;
        for (size_t idx = 0; idx < num_threads; idx++) {
            workers.emplace_back([this, parent, idx] {
                tl_worker_pool = parent;
                tl_worker_id = idx + 1;
                run_worker();
            });
        }
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock{mutex};
            if (closed) {
                return;
            }
            closed = true;
        }
        // This should wake all the workers.
        available.release(ptrdiff_t(workers.size()));
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    bool submit(ThreadPool::Task&& task) {
        {
            std::lock_guard<std::mutex> lock{mutex};
            if (closed) {
                return false;
            }
            queue.push(std::move(task));
        }
        available.release();
        return true;
    }

    void run_worker() {
        while (true) {
            available.acquire();

            ThreadPool::Task task;
            bool wake_next_worker = false;
            {
                std::lock_guard<std::mutex> lock{mutex};
                if (queue.empty()) {
                    if (closed) {
                        return;
                    }
                    continue;
                }

                ThreadPool::Task& next = queue.front();
                if (next.end > 0) {
                    size_t next_start = next_task_part(next);
                    if (next_start < next.end) {
                        // Take part of the task.
                        task = Task{nullptr, next.func_range, next.start, next_start};
                        next.start = next_start;
                        wake_next_worker = true;
                    } else {
                        // Take the remaining part.
                        task = std::move(next);
                        queue.pop();
                    }
                } else {
                    task = std::move(next);
                    queue.pop();
                }
            }

            // The task remains in the queue, wake the next worker before running our part of the task.
            if (wake_next_worker) {
                available.release();
            }
            run_task(task);
        }
    }

    size_t next_task_part(const ThreadPool::Task& task) {
        // Determine the split of task into worker pieces.
        size_t per_worker = task.end / (workers.size() * 32);
        if (per_worker == 0) {
            per_worker = 1;
        }
        return task.start + per_worker;
    }

    void run_task(const ThreadPool::Task& task) {
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

size_t ThreadPool::num_threads() const {
    return impl->workers.size();
}

const char* ThreadPool::name() const {
    return impl->pool_name.c_str();
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
    static ThreadPool thread_pool{"global-pool", std::thread::hardware_concurrency()};
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
    return tl_worker_id;
}
