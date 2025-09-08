#include "common/thread.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <semaphore>
#include <thread>
#include <unordered_set>
#include <vector>

#include "common/sync.h"
#include "common/thread_name.h"
#include "common_test.h"


TEST_SUITE_BEGIN("thread");

TEST_CASE("ThreadPool basic functionality") {
    SUBCASE("create and destroy pool") {
        ThreadPool pool{"", 2};
        CHECK(pool.num_threads() == 2);
    }

    SUBCASE("submit single task") {
        ThreadPool pool{"", 2};
        std::atomic<int> counter = 0;

        bool submitted = pool.submit([&counter]() { counter++; });

        CHECK(submitted);
        pool.shutdown();
        CHECK(counter == 1);
    }

    SUBCASE("submit multiple tasks") {
        ThreadPool pool{"", 2};
        std::atomic<int> counter = 0;
        const int num_tasks = 10;

        for (int i = 0; i < num_tasks; i++) {
            bool submitted = pool.submit([&counter]() { counter++; });
            CHECK(submitted);
        }

        pool.shutdown();
        CHECK(counter == num_tasks);
    }
}

TEST_CASE("ThreadPool submit_for") {
    SUBCASE("submit range of tasks") {
        ThreadPool pool{"", 2};
        const size_t range_size = 100;
        std::vector<std::atomic<bool>> executed(range_size);

        bool submitted = pool.submit_for([&executed](size_t index) { executed[index] = true; }, range_size);

        CHECK(submitted);
        pool.shutdown();

        for (size_t i = 0; i < range_size; i++) {
            CHECK(executed[i]);
        }
    }

    SUBCASE("submit empty range") {
        ThreadPool pool{"", 2};
        std::atomic<int> counter = 0;

        bool submitted = pool.submit_for([&counter](size_t) { counter++; }, 0);

        CHECK(submitted);
        pool.shutdown();
        CHECK(counter == 0);
    }

    SUBCASE("submit range ensures all indices are processed") {
        ThreadPool pool{"", 4};
        const size_t range_size = 1000;
        std::vector<int> results(range_size, 0);

        bool submitted = pool.submit_for([&results](size_t index) { results[index] = int(index * 2); }, range_size);

        CHECK(submitted);
        pool.shutdown();

        for (size_t i = 0; i < range_size; i++) {
            CHECK(results[i] == static_cast<int>(i * 2));
        }
    }
}

TEST_CASE("ThreadPool shutdown behavior") {
    SUBCASE("shutdown waits for tasks to complete") {
        ThreadPool pool{"", 1};
        std::atomic<bool> task_started = false;
        std::atomic<bool> task_completed = false;

        pool.submit([&task_started, &task_completed]() {
            task_started = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            task_completed = true;
        });

        // Give task time to start
        while (!task_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        CHECK(task_started);
        CHECK(!task_completed);

        pool.shutdown();
        CHECK(task_completed);
    }

    SUBCASE("cannot submit after shutdown") {
        ThreadPool pool{"", 2};
        pool.shutdown();

        std::atomic<int> counter = 0;
        bool submitted = pool.submit([&counter]() { counter++; });

        CHECK(!submitted);
        CHECK(counter == 0);
    }

    SUBCASE("shutdown is idempotent") {
        ThreadPool pool{"", 2};
        std::atomic<int> counter = 0;

        pool.submit([&counter]() { counter++; });

        pool.shutdown();
        CHECK(counter == 1);

        // Second shutdown should be safe
        pool.shutdown();
    }
}

TEST_CASE("ThreadPool concurrent execution") {
    SUBCASE("work distribution across threads") {
        ThreadPool pool{"", 4};
        const int num_tasks = 10000;
        std::mutex mutex;
        std::unordered_set<std::thread::id> thread_ids;

        for (int i = 0; i < num_tasks; i++) {
            pool.submit([&mutex, &thread_ids]() {
                std::lock_guard<std::mutex> lock(mutex);
                thread_ids.insert(std::this_thread::get_id());
            });
        }

        pool.shutdown();

        // Should use multiple threads
        CHECK(thread_ids.size() > 1);
        CHECK(thread_ids.size() <= 4);
    }
}

TEST_CASE("ThreadPool local_thread_pool") {
    SUBCASE("main thread is not worker thread") {
        CHECK(local_thread_pool() == nullptr);
        CHECK(local_thread_pool_name() == nullptr);
        CHECK(local_thread_pool_worker_id() == 0);
    }

    SUBCASE("worker threads are identified correctly") {
        ThreadPool pool{"", 2};
        std::atomic<int> worker_count = 0;
        std::atomic<int> non_worker_count = 0;

        for (int i = 0; i < 10; i++) {
            pool.submit([&worker_count, &non_worker_count]() {
                if (local_thread_pool() != nullptr) {
                    worker_count++;
                } else {
                    non_worker_count++;
                }
            });
        }

        pool.shutdown();

        CHECK(worker_count == 10);
        CHECK(non_worker_count == 0);
    }


    SUBCASE("worker thread has name") {
        ThreadPool pool{"my-name", 1};
        std::atomic<const char*> worker_name = nullptr;
        std::atomic<size_t> worker_idx = 0;

        pool.submit([&worker_name, &worker_idx] {
            worker_name = local_thread_pool_name();
            worker_idx = local_thread_pool_worker_id();
        });

        pool.shutdown();

        CHECK(strcmp(worker_name, "my-name") == 0);
        CHECK(worker_idx == 1);
    }
}

TEST_CASE("ThreadPool stress test") {
    SUBCASE("many small tasks") {
        ThreadPool pool{"", 4};
        const int num_tasks = 10000;
        std::atomic<int> counter = 0;

        for (int i = 0; i < num_tasks; i++) {
            pool.submit([&counter]() { counter++; });
        }

        pool.shutdown();
        CHECK(counter == num_tasks);
    }

    SUBCASE("mixed single and range tasks") {
        ThreadPool pool{"", 4};
        std::atomic<int> single_counter = 0;
        std::atomic<int> range_counter = 0;

        // Submit mix of single and range tasks
        for (int i = 0; i < 50; i++) {
            if (i % 2 == 0) {
                pool.submit([&single_counter]() { single_counter++; });
            } else {
                pool.submit_for([&range_counter](size_t) { range_counter++; }, 10);
            }
        }

        pool.shutdown();

        CHECK(single_counter == 25);  // 50/2 single tasks
        CHECK(range_counter == 250);  // 25 * 10 range tasks
    }

    SUBCASE("large range task") {
        ThreadPool pool{"", 8};
        const size_t large_range = 100000;
        std::atomic<size_t> sum = 0;

        pool.submit_for([&sum](size_t index) { sum += index; }, large_range);

        pool.shutdown();

        // Sum of 0..99999 = n*(n-1)/2
        size_t expected_sum = large_range * (large_range - 1) / 2;
        CHECK(sum == expected_sum);
    }

    SUBCASE("call submit inside the thread pool") {
        ThreadPool pool{"", 8};
        const size_t num_outer_tasks = 10000;
        const size_t num_inner_tasks = 100;
        std::atomic<size_t> sum = 0;
        TaskLatch outer_complete{num_outer_tasks};

        pool.submit_for(
            [&sum, &outer_complete](size_t i) {
                thread_pool().submit_for([&sum, i](size_t j) { sum += i * num_inner_tasks + j; }, num_inner_tasks / 2);
                for (size_t j = num_inner_tasks / 2; j < num_inner_tasks; j++) {
                    thread_pool().submit([&sum, i, j] { sum += i * num_inner_tasks + j; });
                }
                outer_complete.count_down();
            },
            num_outer_tasks);

        // Wait until the outer tasks have all submitted their inner tasks.
        outer_complete.wait();

        pool.shutdown();

        size_t total_tasks = num_outer_tasks * num_inner_tasks;
        size_t expected_sum = total_tasks * (total_tasks - 1) / 2;
        CHECK(sum.load() == expected_sum);
    }
}


TEST_CASE("ThreadPool destructor behavior") {
    SUBCASE("destructor calls shutdown") {
        std::atomic<int> counter = 0;
        {
            ThreadPool pool{"", 2};
            for (int i = 0; i < 100; i++) {
                pool.submit([&counter]() { counter++; });
            }
        }
        CHECK(counter == 100);
    }
}

TEST_CASE("ThreadPool with single thread") {
    SUBCASE("single thread pool executes tasks sequentially") {
        ThreadPool pool{"", 1};
        std::vector<int> order;
        std::mutex mutex;

        for (int i = 0; i < 10; i++) {
            pool.submit([&order, &mutex, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                std::lock_guard<std::mutex> lock{mutex};
                order.push_back(i);
            });
        }

        pool.shutdown();

        // Tasks should complete in order for single thread
        CHECK(order.size() == 10);
        for (int i = 0; i < 10; i++) {
            CHECK(order[i] == i);
        }
    }
}

TEST_SUITE_END();
