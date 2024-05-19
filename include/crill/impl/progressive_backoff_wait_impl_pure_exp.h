// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_PURE_EXP_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_PURE_EXP_H

#include <thread>

#if CRILL_INTEL
    #include <emmintrin.h>
    constexpr int PAUSE_TIME = 35; // ns, as benchmarked by Timur
#elif CRILL_ARM_64BIT
    #include <arm_acle.h>
    constexpr int PAUSE_TIME = 1333; // ns, as benchmarked by Timur
#else
    #error "Platform not supported!"
#endif

// does an unrolled loop of _mm_pause() or __wfe() N times
template <unsigned long long N>
constexpr void do_pause() {
    for (unsigned long long i = 0; i < N; ++i) {
        #if CRILL_INTEL
            _mm_pause();
        #elif CRILL_ARM_64BIT
            __wfe();
        #endif
    }
}

#define PAUSE_AND_CHECK(N) \
    if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) > sleep_threshold_ns) { \
        if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) < max_ns) { \
            if (pred()) \
                return; \
            std::this_thread::sleep_for(std::chrono::nanoseconds(PAUSE_TIME * static_cast<unsigned long long>(N))); \
        } else while (true) { \
            if (pred()) \
                return; \
            std::this_thread::sleep_for(std::chrono::nanoseconds(max_ns)); \
        } \
    } else { \
        if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) < max_ns) { \
            if (pred()) \
                return; \
            do_pause<N>(); \
        } else while (true) { \
            if (pred()) \
                return; \
            do_pause<max_ns/PAUSE_TIME>(); \
            std::this_thread::yield(); \
        } \
    }

namespace crill::impl {

template <unsigned long long max_ns, unsigned long long sleep_threshold_ns, typename Predicate>
void progressive_backoff_wait_pure_exp(Predicate&& pred) {
    PAUSE_AND_CHECK(1);
    PAUSE_AND_CHECK(2);
    PAUSE_AND_CHECK(4);
    PAUSE_AND_CHECK(8);
    PAUSE_AND_CHECK(16);
    PAUSE_AND_CHECK(32);
    PAUSE_AND_CHECK(64);
    PAUSE_AND_CHECK(128);
    PAUSE_AND_CHECK(256);
    PAUSE_AND_CHECK(512);
    PAUSE_AND_CHECK(1024);
    PAUSE_AND_CHECK(2048);
    PAUSE_AND_CHECK(4096);
    PAUSE_AND_CHECK(8192);
    PAUSE_AND_CHECK(16384);
    PAUSE_AND_CHECK(32768);
    PAUSE_AND_CHECK(65536);
    PAUSE_AND_CHECK(131072);
    PAUSE_AND_CHECK(262144);
    PAUSE_AND_CHECK(524288);
    PAUSE_AND_CHECK(1048576); // at this point we have 1 MB of just pause statements - should just loop
    PAUSE_AND_CHECK(2097152);
    PAUSE_AND_CHECK(4194304);
    PAUSE_AND_CHECK(8388608);
    PAUSE_AND_CHECK(16777216);
    PAUSE_AND_CHECK(33554432);
    PAUSE_AND_CHECK(67108864);
    PAUSE_AND_CHECK(134217728);
    PAUSE_AND_CHECK(268435456);
    PAUSE_AND_CHECK(536870912);
    PAUSE_AND_CHECK(1073741824);
    PAUSE_AND_CHECK(2147483648);
    PAUSE_AND_CHECK(4294967296);
    PAUSE_AND_CHECK(8589934592);
    PAUSE_AND_CHECK(17179869184);
    PAUSE_AND_CHECK(34359738368);
    PAUSE_AND_CHECK(68719476736);
    PAUSE_AND_CHECK(137438953472);
    PAUSE_AND_CHECK(274877906944);
    PAUSE_AND_CHECK(549755813888);
    PAUSE_AND_CHECK(1099511627776);
}

} // namespace crill::impl

#endif // CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_H
