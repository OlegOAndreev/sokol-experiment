#pragma once

#include <functional>
#include <future>
#include <memory>
#include <utility>

class ThreadPool {
public:
    ThreadPool();
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    void shutdown();

    template <typename F>
    bool submit(F&& task) {
        return submit_impl(std::move(task));
    }

    size_t num_threads() const;

private:
    using Task = std::function<void()>;

    struct Impl;
    std::unique_ptr<Impl> impl;

    bool submit_impl(Task&& task);
};

template <typename F>
auto submit_future(ThreadPool& pool, F&& f) -> std::future<decltype(f())> {
    std::promise<decltype(f())> promise;
    std::future<decltype(f())> future = promise.get_future();
    pool.submit([promise = std::move(promise), f = std::move(f)]() mutable { promise.set_value(f()); });
    return promise.get_future();
}

extern ThreadPool global_thread_pool;
