#include "common/bits.h"

#include <doctest/doctest.h>

TEST_SUITE_BEGIN("bits");

TEST_CASE("is_pow2") {
    SUBCASE("zero") {
        CHECK(!is_pow2(0));
    }

    SUBCASE("powers of two") {
        CHECK(is_pow2(1));
        CHECK(is_pow2(2));
        CHECK(is_pow2(512));
    }

    SUBCASE("not powers of two") {
        CHECK(!is_pow2(3));
        CHECK(!is_pow2(5));
        CHECK(!is_pow2(7));
        CHECK(!is_pow2(9));
    }

    SUBCASE("maximum values") {
        CHECK(!is_pow2(0x7FFFFFFF));
        CHECK(is_pow2(0x80000000));
        CHECK(!is_pow2(0xFFFFFFFF));
        if (SIZE_T_IS_64_BIT) {
            CHECK(!is_pow2(0x7FFFFFFFFFFFFFFF));
            CHECK(is_pow2(0x8000000000000000));
            CHECK(!is_pow2(0xFFFFFFFFFFFFFFFF));
        }
    }
}

TEST_CASE("next_log2") {
    SUBCASE("zero") {
        CHECK(next_log2(0) == 0);
        CHECK(next_log2(0) == 0);
    }

    SUBCASE("powers of two") {
        CHECK(next_log2(1) == 1);
        CHECK(next_log2_inclusive(1) == 0);
        CHECK(next_log2(2) == 2);
        CHECK(next_log2_inclusive(2) == 1);
        CHECK(next_log2(512) == 10);
        CHECK(next_log2_inclusive(512) == 9);
    }

    SUBCASE("values between powers of two") {
        CHECK(next_log2(3) == 2);
        CHECK(next_log2_inclusive(3) == 2);
        CHECK(next_log2(5) == 3);
        CHECK(next_log2_inclusive(5) == 3);
        CHECK(next_log2(7) == 3);
        CHECK(next_log2_inclusive(7) == 3);
        CHECK(next_log2(9) == 4);
        CHECK(next_log2_inclusive(9) == 4);
    }

    SUBCASE("maximum values") {
        CHECK(next_log2(0x7FFFFFFF) == 31);
        CHECK(next_log2_inclusive(0x7FFFFFFF) == 31);
        CHECK(next_log2(0x80000000) == 32);
        CHECK(next_log2_inclusive(0x80000000) == 31);
        CHECK(next_log2(0xFFFFFFFF) == 32);
        CHECK(next_log2_inclusive(0xFFFFFFFF) == 32);
        if (SIZE_T_IS_64_BIT) {
            CHECK(next_log2(0x7FFFFFFFFFFFFFFF) == 63);
            CHECK(next_log2_inclusive(0x7FFFFFFFFFFFFFFF) == 63);
            CHECK(next_log2(0x8000000000000000) == 64);
            CHECK(next_log2_inclusive(0x8000000000000000) == 63);
            CHECK(next_log2(0xFFFFFFFFFFFFFFFF) == 64);
            CHECK(next_log2_inclusive(0xFFFFFFFFFFFFFFFF) == 64);
        }
    }

    SUBCASE("monotonicity") {
        size_t prev = 0;
        int prev_result = next_log2(prev);

        for (size_t i = 1; i < 100000; i++) {
            int current_result = next_log2(i);
            CHECK(current_result >= prev_result);
            prev_result = current_result;
        }
    }

    SUBCASE("boundary consistency") {
        for (size_t v = 1; v < 100000; v++) {
            int e = next_log2(v);
            int e_inclusive = next_log2_inclusive(v);
            size_t lower_bound = (e > 0) ? (1ULL << (e - 1)) : 0;
            size_t upper_bound = (1ULL << e);
            size_t lower_bound_inclusive = (e_inclusive > 0) ? (1ULL << (e_inclusive - 1)) : 0;
            size_t upper_bound_inclusive = (1ULL << e_inclusive);

            CHECK(v >= lower_bound);
            CHECK(v < upper_bound);
            CHECK(v > lower_bound_inclusive);
            CHECK(v <= upper_bound_inclusive);
        }
    }
}

TEST_SUITE_END();
