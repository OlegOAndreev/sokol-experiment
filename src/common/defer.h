#pragma once

// See https://www.balintkissdev.com/blog/adding-defer-keyword-to-cpp/. Added _My to avoid collisions with standard
// symbols.
template <typename F>
struct _MyScopedDefer {
    _MyScopedDefer(F f) : f(f) {
    }

    ~_MyScopedDefer() {
        f();
    }

    F f;
};

#define _MY_SCOPED_DEFER_STR_CONCAT(a, b) _MY_SCOPED_DEFER_DO_STR_CONCAT(a, b)
#define _MY_SCOPED_DEFER_DO_STR_CONCAT(a, b) a##b

#define DEFER(x) const auto _MY_SCOPED_DEFER_STR_CONCAT(_my_scoped_defer_var_, __LINE__) = _MyScopedDefer([&]() { x; })

// Helper to make using DEFER more ergonomic: replace v with default value and return the previous value. Inspired by
// https://doc.rust-lang.org/std/mem/fn.take.html
template <typename T>
T move_from(T& v) {
    T result = static_cast<T&&>(v);
    v = T{};
    return result;
}
