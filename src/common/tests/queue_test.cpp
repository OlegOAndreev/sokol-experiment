#include "common/queue.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "common/struct.h"
#include "common_test.h"


TEST_SUITE_BEGIN("queue");

TEST_CASE("Queue basic operations") {
    SUBCASE("newly created queue is empty") {
        Queue<int> q;
        CHECK(q.empty());
        CHECK(q.size() == 0);
    }

    SUBCASE("push and pop single element") {
        Queue<int> q;
        q.push(42);
        CHECK(!q.empty());
        CHECK(q.size() == 1);
        CHECK(q.front() == 42);

        q.pop();
        CHECK(q.empty());
        CHECK(q.size() == 0);
    }

    SUBCASE("push multiple elements maintains FIFO order") {
        Queue<int> q;
        q.push(1);
        q.push(2);
        q.push(3);

        CHECK(q.size() == 3);
        CHECK(q.front() == 1);
        q.pop();
        CHECK(q.front() == 2);
        q.pop();
        CHECK(q.front() == 3);
        q.pop();
        CHECK(q.empty());
    }
}

TEST_CASE("Queue with custom capacity") {
    SUBCASE("fill up to capacity") {
        Queue<int> q(4);
        q.push(1);
        q.push(2);
        q.push(3);
        q.push(4);

        CHECK(q.size() == 4);
        CHECK(q.front() == 1);
    }

    SUBCASE("exceeding capacity triggers resize") {
        Queue<int> q(4);
        for (int i = 0; i < 5; i++) {
            q.push(i);
        }

        CHECK(q.size() == 5);
        for (int i = 0; i < 5; i++) {
            CHECK(q.front() == i);
            q.pop();
        }
        CHECK(q.empty());
    }
}

TEST_CASE("Queue circular buffer behavior") {
    SUBCASE("wrap around without resize") {
        Queue<int> q(4);
        q.push(1);
        q.push(2);
        q.push(3);
        q.pop();
        q.pop();
        q.push(4);
        q.push(5);
        q.push(6);

        CHECK(q.size() == 4);
        CHECK(q.front() == 3);
        q.pop();
        CHECK(q.front() == 4);
        q.pop();
        CHECK(q.front() == 5);
        q.pop();
        CHECK(q.front() == 6);
        q.pop();
        CHECK(q.empty());
    }

    SUBCASE("resize when wrapped") {
        Queue<int> q(4);
        q.push(1);
        q.push(2);
        q.push(3);
        q.pop();
        q.pop();
        q.push(4);
        q.push(5);
        q.push(6);
        q.push(7);

        CHECK(q.size() == 5);

        CHECK(q.front() == 3);
        q.pop();
        CHECK(q.front() == 4);
        q.pop();
        CHECK(q.front() == 5);
        q.pop();
        CHECK(q.front() == 6);
        q.pop();
        CHECK(q.front() == 7);
        q.pop();
        CHECK(q.empty());
    }
}

TEST_CASE("Queue with move semantics") {
    Queue<MoveOnly> q;

    SUBCASE("move objects into queue") {
        q.push(MoveOnly(42));
        CHECK(q.front().value == 42);

        MoveOnly obj(100);
        q.push(std::move(obj));
        CHECK(obj.value == -1);

        q.pop();
        CHECK(q.front().value == 100);
    }
}

TEST_CASE("Queue with copyable objects") {
    SUBCASE("copy and move strings") {
        Queue<std::string> q;
        std::string s1 = "hello";
        q.push(s1);
        CHECK(s1 == "hello");
        CHECK(q.front() == "hello");

        std::string s2 = "world";
        q.push(std::move(s2));
        CHECK(s2.empty());

        q.pop();
        CHECK(q.front() == "world");
    }

    SUBCASE("multiple strings with resize") {
        Queue<std::string> q;
        std::vector<std::string> strings = {"alpha", "beta", "gamma", "delta", "epsilon",
                                            "zeta",  "eta",  "theta", "iota",  "kappa"};

        for (const std::string& s : strings) {
            q.push(s);
        }

        CHECK(q.size() == strings.size());

        for (const std::string& expected : strings) {
            CHECK(q.front() == expected);
            q.pop();
        }

        CHECK(q.empty());
    }
}

TEST_CASE("Queue stress test") {
    Queue<int> q;
    const int iterations = 1000;

    SUBCASE("push and pop many elements") {
        for (int i = 0; i < iterations; i++) {
            q.push(i);
        }

        CHECK(q.size() == iterations);

        for (int i = 0; i < iterations; i++) {
            CHECK(q.front() == i);
            q.pop();
        }

        CHECK(q.empty());
    }

    SUBCASE("interleaved push and pop") {
        int pushed = 0;
        int popped = 0;

        for (int i = 0; i < 10; i++) {
            q.push(pushed);
            pushed++;
        }

        for (int round = 0; round < 100; round++) {
            for (int i = 0; i < 5 && !q.empty(); i++) {
                CHECK(q.front() == popped);
                q.pop();
                popped++;
            }

            for (int i = 0; i < 7; i++) {
                q.push(pushed);
                pushed++;
            }
        }

        while (!q.empty()) {
            CHECK(q.front() == popped);
            q.pop();
            popped++;
        }

        CHECK(pushed == popped);
    }
}

TEST_CASE("Queue edge cases") {
    SUBCASE("queue with capacity 1") {
        Queue<int> q(1);
        q.push(42);
        CHECK(q.front() == 42);
        q.pop();
        CHECK(q.empty());
        q.push(1);
        q.push(2);
        CHECK(q.size() == 2);
    }

    SUBCASE("repeated push/pop at boundary") {
        Queue<int> q(2);

        for (int round = 0; round < 10; round++) {
            q.push(round * 3);
            q.push(round * 3 + 1);
            q.push(round * 3 + 2);
            CHECK(q.size() == 3);
            CHECK(q.front() == round * 3);
            q.pop();
            CHECK(q.front() == round * 3 + 1);
            q.pop();
            CHECK(q.front() == round * 3 + 2);
            q.pop();
            CHECK(q.empty());
        }
    }
}

TEST_SUITE_END();
