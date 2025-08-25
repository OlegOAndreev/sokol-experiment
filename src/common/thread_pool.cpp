#include "thread_pool.h"

#include <thread>

#include "common/mt_queue.h"


ThreadPool global_thread_pool;

struct ThreadPool::Impl {
    std::vector<std::thread> workers;
    MtQueue<ThreadPool::Task> queue;

    void init(size_t num_threads) {
        for (size_t i = 0; i < num_threads; i++) {
            workers.push_back(std::thread(&ThreadPool::Impl::run_worker, this));
        }
    }

    void shutdown() {
        queue.close();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    bool submit(std::function<void()>&& task) {
        return queue.push(std::move(task));
    }

    void run_worker() {
        while (true) {
            ThreadPool::Task task;
            if (!queue.pop(&task)) {
                return;
            }
            task();
        }
    }
};

ThreadPool::ThreadPool() : ThreadPool(std::thread::hardware_concurrency()) {
}

ThreadPool::ThreadPool(size_t num_threads) : impl(new ThreadPool::Impl()) {
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
