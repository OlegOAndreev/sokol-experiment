#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include "common.h"


// Set either SIZE_T_IS_32_BIT or SIZE_T_IS_64_BIT to 1.
#if SIZE_MAX == 0xFFFFFFFF
#define SIZE_T_IS_32_BIT 1
#elif SIZE_MAX == 0xFFFFFFFFFFFFFFFF
#define SIZE_T_IS_64_BIT 1
#else
#error "Unsupported SIZE_MAX"
#endif


// Return true if v is 2^k for some k.
FORCE_INLINE bool is_pow2(size_t v) {
    return (v & (v - 1)) == 0 && v != 0;
}

// Return the exponent e such that 2^(e - 1) <= v < 2^e.
FORCE_INLINE int next_log2(size_t v) {
#if defined(__clang__) || defined(__GNUC__)
    if (v == 0) {
        return 0;
    } else {
#if SIZE_T_IS_32_BIT
        return sizeof(size_t) * 8 - __builtin_clz(v);
#elif SIZE_T_IS_64_BIT
        return int(sizeof(size_t)) * 8 - __builtin_clzl(v);
#else
#error "Unsupported COMMON_SIZE_T_BITS"
#endif
    }
#elif defined(_MSC_VER)
    unsigned char nonzero;
    unsigned long index;
#if SIZE_T_IS_32_BIT
    nonzero = _BitScanReverse(&index, v);
#elif SIZE_T_IS_64_BIT
    nonzero = _BitScanReverse64(&index, v);
#else
#error "Unsupported COMMON_SIZE_T_BITS"
#endif
    if (nonzero) {
        return index + 1;
    } else {
        return 0;
    }
#else
    int e = 0;
    while (v != 0) {
        e++;
        v >>= 1;
    }
    return e;
#endif
}

// Return the next 2^k such that v < 2^k.
FORCE_INLINE size_t next_pow2(size_t v) {
    return 1 << next_log2(v);
}

// Return the exponent e such that 2^(e - 1) < v <= 2^e.
FORCE_INLINE int next_log2_inclusive(size_t v) {
    if (is_pow2(v)) {
        return next_log2(v) - 1;
    } else {
        return next_log2(v);
    }
}

// Return the next 2^k such that v <= 2^k.
FORCE_INLINE size_t next_pow2_inclusive(size_t v) {
    return 1 << next_log2_inclusive(v);
}
