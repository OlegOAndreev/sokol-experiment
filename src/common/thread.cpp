#include "thread.h"

#include <sokol_slog.h>

#include <cstddef>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

#include "queue.h"


ThreadPool g_thread_pool;

struct ThreadPool::Impl {
    std::vector<std::thread> workers;

    std::counting_semaphore<> available{0};
    std::mutex mutex;
    Queue<ThreadPool::Task> queue;
    bool closed = false;

    static thread_local Impl* tl_worker_pool;

    void init(size_t num_threads) {
        for (size_t i = 0; i < num_threads; i++) {
            workers.emplace_back([this] {
                tl_worker_pool = this;
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

thread_local ThreadPool::Impl* ThreadPool::Impl::tl_worker_pool = nullptr;

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {
}

ThreadPool::ThreadPool(size_t num_threads) : impl(new Impl()) {
    impl->init(num_threads);
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

bool ThreadPool::submit_impl(Task&& task) {
    return impl->submit(std::move(task));
}

bool ThreadPool::is_worker_thread() {  // static
    return Impl::tl_worker_pool != nullptr;
}
