#include "thread.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "queue.h"
#include "slog.h"
#include "thread_name.h"


thread_local ThreadPool* tl_worker_pool = nullptr;
thread_local size_t tl_worker_idx = 0;

struct ThreadPoolWorker {
    std::thread thread;
};

struct ThreadPool::Impl {
    std::string name;
    size_t max_queue_size = 0;
    std::vector<ThreadPoolWorker> workers;

    std::mutex mutex;
    std::condition_variable available;
    Queue<ThreadPool::Task> queue;
    bool closed = false;

    void init(const char* name, size_t num_threads, size_t max_queue_size, ThreadPool* parent) {
        this->name = name;
        this->max_queue_size = max_queue_size;
        for (size_t idx = 0; idx < num_threads; idx++) {
            std::thread worker_thread{[this, parent, idx] {
                tl_worker_pool = parent;
                tl_worker_idx = idx;
                run_worker();
            }};
            workers.emplace_back(std::move(worker_thread));
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
        available.notify_all();
        for (ThreadPoolWorker& worker : workers) {
            if (worker.thread.joinable()) {
                worker.thread.join();
            }
        }
    }

    bool submit(ThreadPool::Task* tasks, size_t num_tasks) {
        {
            std::lock_guard<std::mutex> lock{mutex};
            if (closed) {
                return false;
            }
            if (max_queue_size != 0 && num_tasks + queue.size() > max_queue_size) {
                return false;
            }
            for (size_t i = 0; i < num_tasks; i++) {
                queue.push(std::move(tasks[i]));
            }
        }
        if (num_tasks == 1) {
            available.notify_one();
        } else {
            available.notify_all();
        }
        return true;
    }

    void run_worker() {
        while (true) {
            ThreadPool::Task task;
            bool wake_next_worker = false;
            {
                std::unique_lock<std::mutex> lock{mutex};
                while (queue.empty()) {
                    if (closed) {
                        return;
                    }
                    available.wait(lock);
                }

                ThreadPool::Task& next = queue.front();
                if (next.end > 0) {
                    size_t next_start = next_task_part(next);
                    if (next_start < next.end) {
                        // Take part of the task.
                        task = next;
                        task.end = next_start;
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
                available.notify_one();
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
        ptrdiff_t count = 0;
        if (task.end > 0) {
            for (size_t i = task.start; i < task.end; i++) {
                task.func_range(i);
            }
            count = ptrdiff_t(task.end - task.start);
        } else {
            task.func_single();
            count = 1;
        }
        if (task.latch != nullptr) {
            task.latch->decrement(count);
        }
    }

    void wait_impl(Latch* latch) {
        latch->wait();
        // while (!latch->try_wait()) {
        //     // Try popping task from the queue.
        //     // Ignore closed.
        //     // If queue is empty, replace latch in worker. Register worker as waiter.
        //     // Wait for latch.
        //     // Replace latch with in worker with standard. Unregister worker as waiter.


        //     // TODO: REPLACE
        //     ThreadPool::Task task;
        //     bool wake_next_worker = false;
        //     {
        //         std::lock_guard<std::mutex> lock{mutex};
        //         if (queue.empty()) {
        //             if (closed) {
        //                 return;
        //             }
        //             continue;
        //         }

        //         ThreadPool::Task& next = queue.front();
        //         if (next.end > 0) {
        //             size_t next_start = next_task_part(next);
        //             if (next_start < next.end) {
        //                 // Take part of the task.
        //                 task = next;
        //                 task.end = next_start;
        //                 next.start = next_start;
        //                 wake_next_worker = true;
        //             } else {
        //                 // Take the remaining part.
        //                 task = std::move(next);
        //                 queue.pop();
        //             }
        //         } else {
        //             task = std::move(next);
        //             queue.pop();
        //         }
        //     }

        //     // The task remains in the queue, wake the next worker before running our part of the task.
        //     if (wake_next_worker) {
        //         available.notify_one();
        //     }
        //     run_task(task);
        // }
    }
};

ThreadPool::Latch::Latch(size_t count) {
}

void ThreadPool::Latch::decrement(size_t count) {
}

void ThreadPool::Latch::wait() {
}

ThreadPool::ThreadPool(const char* name, size_t num_threads, size_t max_queue_size) : impl(new Impl()) {
    impl->init(name, num_threads, max_queue_size, this);
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
    return impl->name.c_str();
}

bool ThreadPool::submit_impl(ThreadPool::Task* tasks, size_t num_tasks) {
    return impl->submit(tasks, num_tasks);
}

void ThreadPool::wait_impl(ThreadPool::Latch* latch) {  // static
    ThreadPool* pool = local_thread_pool();
    if (pool == nullptr) {
        // The thread is outside the thread pool, do a blocking wait.
        latch->wait();
    } else {
        pool->impl->wait_impl(latch);
    }
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
    if (tl_worker_pool == nullptr) {
        return 0;
    }
    return tl_worker_idx + 1;
}
