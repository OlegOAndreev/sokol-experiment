#pragma once

#include "common/struct.h"

struct MoveOnly {
    int value;

    MoveOnly() : value(0) {
    }

    explicit MoveOnly(int v) : value(v) {
    }

    MoveOnly(MoveOnly&& other) noexcept : value(other.value) {
        other.value = -1;
    }

    MoveOnly& operator=(MoveOnly&& other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }

    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    ~MoveOnly() {
    }
};
