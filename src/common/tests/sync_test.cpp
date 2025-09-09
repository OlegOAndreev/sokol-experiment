#include "common/sync.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <thread>
#include <vector>

#include "common_test.h"


TEST_SUITE_BEGIN("sync");

TEST_CASE("TaskLatch basic operations") {
    SUBCASE("newly created latch is not done") {
        TaskLatch latch(1);
        CHECK(!latch.done());
    }

    SUBCASE("count down to zero makes latch done") {
        TaskLatch latch(1);
        latch.count_down();
        CHECK(latch.done());
    }

    SUBCASE("count down with custom amount") {
        TaskLatch latch(3);
        latch.count_down(2);
        CHECK(!latch.done());
        latch.count_down(1);
        CHECK(latch.done());
    }

    SUBCASE("wait returns immediately when already done") {
        TaskLatch latch(1);
        latch.count_down();
        latch.wait();
        CHECK(latch.done());
    }
}

TEST_CASE("TaskLatch multi-threaded wait") {
    SUBCASE("thread waits for count down") {
        TaskLatch latch(1);
        bool thread_completed = false;

        std::thread waiter([&] {
            latch.wait();
            thread_completed = true;
        });

        // Give the waiter thread time to start waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        CHECK(!thread_completed);
        latch.count_down();

        waiter.join();
        CHECK(thread_completed);
    }

    SUBCASE("multiple threads wait for completion") {
        TaskLatch latch(2);
        int completed_threads = 0;
        std::mutex mutex;

        auto worker = [&] {
            latch.wait();
            std::lock_guard<std::mutex> lock(mutex);
            completed_threads++;
        };

        std::thread t1(worker);
        std::thread t2(worker);

        // Give threads time to start waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        CHECK(completed_threads == 0);
        latch.count_down(2);

        t1.join();
        t2.join();

        CHECK(completed_threads == 2);
    }
}

TEST_CASE("MPMCQueue basic operations") {
    SUBCASE("newly created queue is empty") {
        MPMCQueue<int> q;
        CHECK(q.empty());
        CHECK(q.size() == 0);
    }

    SUBCASE("push and pop single element") {
        MPMCQueue<int> q;
        CHECK(q.try_push(42));
        CHECK(!q.empty());
        CHECK(q.size() == 1);

        int value = 0;
        CHECK(q.try_pop(value));
        CHECK(value == 42);
        CHECK(q.empty());
        CHECK(q.size() == 0);
    }

    SUBCASE("push and pop multiple elements maintains FIFO order") {
        MPMCQueue<int> q;
        q.push(1);
        q.push(2);
        q.push(3);

        CHECK(q.size() == 3);

        int value = 0;
        CHECK(q.pop(value));
        CHECK(value == 1);
        CHECK(q.pop(value));
        CHECK(value == 2);
        CHECK(q.pop(value));
        CHECK(value == 3);
        CHECK(q.empty());
    }
}

TEST_CASE("MPMCQueue with capacity") {
    SUBCASE("fill up to capacity") {
        MPMCQueue<int> q(4);
        CHECK(q.try_push(1));
        CHECK(q.try_push(2));
        CHECK(q.try_push(3));
        CHECK(q.try_push(4));

        CHECK(q.size() == 4);

        int value = 0;
        CHECK(q.try_pop(value));
        CHECK(value == 1);
    }

    SUBCASE("exceeding capacity still works (unbounded)") {
        MPMCQueue<int> q(4);
        for (int i = 0; i < 10; i++) {
            CHECK(q.try_push(i));
        }

        CHECK(q.size() == 10);
        for (int i = 0; i < 10; i++) {
            int value = 0;
            CHECK(q.try_pop(value));
            CHECK(value == i);
        }
        CHECK(q.empty());
    }
}

TEST_CASE("MPMCQueue close operations") {
    SUBCASE("try_push returns false after close") {
        MPMCQueue<int> q;
        q.close();
        CHECK(!q.try_push(42));
        CHECK(q.empty());
    }

    SUBCASE("pop returns false after close when empty") {
        MPMCQueue<int> q;
        q.close();

        int value = 0;
        CHECK(!q.pop(value));
    }

    SUBCASE("pop returns true after close when not empty") {
        MPMCQueue<int> q;
        q.push(42);
        q.close();

        int value = 0;
        CHECK(q.pop(value));
        CHECK(value == 42);
        CHECK(q.empty());

        // Subsequent pops should return false
        CHECK(!q.pop(value));
    }

    SUBCASE("try_pop returns true after close when not empty") {
        MPMCQueue<int> q;
        q.push(42);
        q.close();

        int value = 0;
        CHECK(q.try_pop(value));
        CHECK(value == 42);
        CHECK(q.empty());

        // Subsequent pops should return false
        CHECK(!q.try_pop(value));
    }
}

TEST_CASE("MPMCQueue with move semantics") {
    SUBCASE("move objects into queue") {
        MPMCQueue<MoveOnly> q;
        q.push(MoveOnly(42));

        MoveOnly obj;
        CHECK(q.pop(obj));
        CHECK(obj.value == 42);
    }

    SUBCASE("try_push with move semantics") {
        MPMCQueue<MoveOnly> q;
        MoveOnly obj(100);
        q.push(std::move(obj));
        CHECK(obj.value == -1);  // Object should be moved from

        MoveOnly result;
        CHECK(q.pop(result));
        CHECK(result.value == 100);
    }
}

TEST_CASE("MPMCQueue multi-threaded basic") {
    SUBCASE("single producer, single consumer") {
        MPMCQueue<int> q;
        const int count = 10000;

        std::thread producer([&] {
            for (int i = 0; i < count; i++) {
                q.push(i);
            }
            q.close();
        });

        std::thread consumer([&] {
            int value = 0;
            for (int i = 0; q.pop(value); i++) {
                CHECK(value == i);
            }
        });

        producer.join();
        consumer.join();

        CHECK(q.empty());
    }
}

TEST_CASE("MPMCQueue multi-threaded stress test") {
    SUBCASE("multiple producers, single consumer") {
        MPMCQueue<int> q;
        const int producers = 4;
        const int items_per_producer = 10000;
        const int total_items = producers * items_per_producer;

        std::vector<std::thread> producer_threads;
        std::atomic<int> produced_count = 0;

        for (int i = 0; i < producers; i++) {
            producer_threads.emplace_back([&, producer_id = i] {
                for (int j = 0; j < items_per_producer; j++) {
                    int value = producer_id * items_per_producer + j;
                    q.push(value);
                    produced_count++;
                }
            });
        }

        std::vector<int> consumed;
        consumed.reserve(total_items);

        std::thread consumer([&] {
            for (int i = 0; i < total_items; i++) {
                int value = 0;
                CHECK(q.pop(value));
                consumed.push_back(value);
            }
        });

        for (auto& thread : producer_threads) {
            thread.join();
        }
        consumer.join();

        CHECK(produced_count == total_items);
        CHECK(consumed.size() == total_items);
        CHECK(q.empty());

        std::sort(consumed.begin(), consumed.end());
        for (int i = 0; i < total_items; i++) {
            CHECK(consumed[i] == i);
        }
    }
}

TEST_CASE("MPMCQueue edge cases") {
    SUBCASE("try_pop on empty queue returns false") {
        MPMCQueue<int> q;
        int value = 0;
        CHECK(!q.try_pop(value));
    }

    SUBCASE("queue with capacity 1") {
        MPMCQueue<int> q(1);
        CHECK(q.try_push(42));
        CHECK(q.size() == 1);

        int value = 0;
        CHECK(q.try_pop(value));
        CHECK(value == 42);
        CHECK(q.empty());

        // Should be able to push more even with capacity 1
        CHECK(q.try_push(1));
        CHECK(q.try_push(2));  // This should trigger internal resize
        CHECK(q.size() == 2);
    }

    SUBCASE("concurrent close with active producers") {
        MPMCQueue<int> q;
        std::atomic<bool> running = true;

        std::thread producer([&] {
            int i = 0;
            while (running) {
                if (!q.try_push(i++)) {
                    break;  // Queue was closed
                }
            }
        });

        // Let producer run for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        q.close();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        running = false;

        producer.join();

        // Should be able to consume remaining items
        int value = 0;
        while (q.try_pop(value)) {
            // Consume all items
        }
        CHECK(q.empty());
    }
}

TEST_SUITE_END();
