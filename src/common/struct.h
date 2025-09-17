#pragma once

// Disable move and copy
#define DISABLE_MOVE_AND_COPY(T)     \
    T(const T&) = delete;            \
    T(T&&) = delete;                 \
    T& operator=(const T&) = delete; \
    T& operator=(T&&) = delete;

// Disable copy (and default move constructor to silence cpp-tidy).
#define DISABLE_COPY(T)              \
    T(const T&) = delete;            \
    T& operator=(const T&) = delete;
