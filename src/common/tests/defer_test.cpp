#include "common/defer.h"

#include <snitch/snitch.hpp>
#include <string>
#include <vector>


TEST_CASE("DEFER basic functionality") {
    SECTION("defer executes at scope end") {
        int value = 0;
        {
            CHECK(value == 0);
            DEFER(value = 42);
            CHECK(value == 0);
        }
        CHECK(value == 42);
    }

    SECTION("defer executes in reverse order") {
        std::vector<int> order;
        {
            DEFER(order.push_back(3));
            DEFER(order.push_back(2));
            DEFER(order.push_back(1));
            CHECK(order.empty());
        }
        CHECK(order.size() == 3);
        CHECK(order[0] == 1);
        CHECK(order[1] == 2);
        CHECK(order[2] == 3);
    }

    SECTION("defer captures by reference") {
        int a = 10;
        int b = 20;
        {
            DEFER(a = b);
            b = 30;
            CHECK(a == 10);
        }
        CHECK(a == 30);
    }

    SECTION("defer in loop") {
        std::vector<int> values;

        for (int i = 0; i < 3; ++i) {
            DEFER(values.push_back(i));
        }

        CHECK(values.size() == 3);
        CHECK(values[0] == 0);
        CHECK(values[1] == 1);
        CHECK(values[2] == 2);
    }
}

TEST_CASE("DEFER with multiple statements") {
    SECTION("defer can execute multiple statements") {
        int counter = 0;
        std::string message;
        {
            DEFER({
                counter++;
                message = "executed";
            });
            CHECK(counter == 0);
            CHECK(message.empty());
        }
        CHECK(counter == 1);
        CHECK(message == "executed");
    }
}

TEST_CASE("DEFER in nested scopes") {
    SECTION("nested defers execute in correct order") {
        std::vector<int> order;
        {
            DEFER(order.push_back(4));
            {
                DEFER(order.push_back(2));
                DEFER(order.push_back(1));
            }
            CHECK(order.size() == 2);
            CHECK(order[0] == 1);
            CHECK(order[1] == 2);

            DEFER(order.push_back(3));
        }
        CHECK(order.size() == 4);
        CHECK(order[0] == 1);
        CHECK(order[1] == 2);
        CHECK(order[2] == 3);
        CHECK(order[3] == 4);
    }
}

TEST_CASE("DEFER with function calls") {
    SECTION("defer can call functions") {
        int cleanup_count = 0;
        auto cleanup = [&cleanup_count] { cleanup_count++; };

        {
            DEFER(cleanup());
            CHECK(cleanup_count == 0);
        }
        CHECK(cleanup_count == 1);
    }

    SECTION("defer with member function calls") {
        struct Resource {
            bool is_open = true;
            int close_count = 0;

            void close() {
                is_open = false;
                close_count++;
            }
        };

        Resource resource;
        {
            CHECK(resource.is_open);
            DEFER(resource.close());
            CHECK(resource.is_open);
        }
        CHECK(!resource.is_open);
        CHECK(resource.close_count == 1);
    }
}

TEST_CASE("DEFER with early returns") {
    SECTION("defer executes even with early return") {
        auto test_func = [](bool early_return) {
            int value = 0;
            {
                DEFER(value = 100);
                if (early_return) {
                    return value;
                }
                value = 50;
            }
            return value;
        };

        CHECK(test_func(true) == 0);
        CHECK(test_func(false) == 100);
    }
}

TEST_CASE("DEFER with exceptions") {
    SECTION("defer executes during exception unwinding") {
        int cleanup_count = 0;

        auto throwing_func = [&cleanup_count]() {
            DEFER(cleanup_count++);
            throw std::runtime_error("test exception");
        };

        CHECK_THROWS_AS(throwing_func(), std::runtime_error);
        CHECK(cleanup_count == 1);
    }

    SECTION("multiple defers during exception") {
        std::vector<int> order;

        auto throwing_func = [&order]() {
            DEFER(order.push_back(3));
            DEFER(order.push_back(2));
            DEFER(order.push_back(1));
            throw std::runtime_error("test exception");
        };

        CHECK_THROWS_AS(throwing_func(), std::runtime_error);
        CHECK(order.size() == 3);
        CHECK(order[0] == 1);
        CHECK(order[1] == 2);
        CHECK(order[2] == 3);
    }
}

TEST_CASE("move_from") {
    SECTION("move_from with int") {
        int value = 42;
        int moved = move_from(value);
        CHECK(moved == 42);
        CHECK(value == 0);
    }

    SECTION("move_from with string") {
        std::string str = "hello";
        std::string moved = move_from(str);
        CHECK(moved == "hello");
        CHECK(str.empty());
    }

    SECTION("move_from with custom struct") {
        struct Point {
            int x = 0;
            int y = 0;
        };

        Point p{10, 20};
        Point moved = move_from(p);
        CHECK(moved.x == 10);
        CHECK(moved.y == 20);
        CHECK(p.x == 0);
        CHECK(p.y == 0);
    }

    SECTION("move_from with pointer") {
        int* ptr = new int(42);
        int* moved = move_from(ptr);
        CHECK(*moved == 42);
        CHECK(ptr == nullptr);
        delete moved;
    }
}
