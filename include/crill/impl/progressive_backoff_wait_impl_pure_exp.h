// forceinline with -O0??

// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_PURE_EXP_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_PURE_EXP_H

#include <thread>
#include <cmath>
#include <iostream>

#if defined(_MSC_VER)
    #define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define FORCEINLINE __attribute__((always_inline)) inline
#else
    #error "Compiler not supported!"
#endif

#if CRILL_ARM_64BIT
    #if defined(_MSC_VER)
        #include <intrin.h>
        #define PAUSE_ASM() if constexpr (use_isb) { __isb(0); } else { __wfe(); }
    #elif defined(__GNUC__) || defined(__clang__)
        #define PAUSE_ASM() if constexpr (use_isb) { asm volatile ("isb"); } else { asm volatile ("wfe"); }
    #endif
#elif CRILL_INTEL
    #if defined(_MSC_VER)
        #include <emmintrin.h>
        #define PAUSE_ASM() _mm_pause();
    #elif defined(__GNUC__) || defined(__clang__)
        #define PAUSE_ASM() asm volatile ("pause")
    #endif
#else
    #error "Platform not supported!"
#endif

// partly unrolled loop
template<unsigned long long N, bool use_isb, unsigned long long max_unroll=64>
FORCEINLINE constexpr void do_pause() {
    if constexpr(N > max_unroll) {
        for (unsigned long long i = 0; i < N / max_unroll; i++) {
            do_pause<max_unroll, use_isb, max_unroll>();
        }
        do_pause<N % max_unroll, use_isb, max_unroll>();
    }
    else {
        if constexpr (N > 1ULL) {
            do_pause<N / 2ULL, use_isb, max_unroll>();
            do_pause<N / 2ULL, use_isb, max_unroll>();
            do_pause<N % 2ULL, use_isb, max_unroll>();
        } else if constexpr (N == 1ULL) {
            PAUSE_ASM();
        }
    }
}

namespace crill::impl {

template <unsigned long long min_ns, unsigned long long max_ns, unsigned long long sleep_threshold_ns, bool use_isb, typename Predicate>
void progressive_backoff_wait_pure_exp(Predicate&& pred) {
    // set pause time based on platform and use_isb
    #if CRILL_INTEL
        constexpr unsigned long long int PAUSE_TIME = 35;
    #elif CRILL_ARM_64BIT
        constexpr unsigned long long int PAUSE_TIME = (use_isb) ? 10 : 970;
    #endif

    #define PAUSE_AND_CHECK(N) \
        if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) >= min_ns) { \
            if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) >= sleep_threshold_ns) { \
                if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) < max_ns) { \
                /*ASM_("nop; nop; nop; nop; nop;"); */ \
                    if (pred()) \
                        return; \
                    std::this_thread::sleep_for(std::chrono::nanoseconds(PAUSE_TIME * static_cast<unsigned long long>(N))); \
                } else while (true) { \
                /*ASM_("nop; nop; nop; nop; nop; nop"); */ \
                    if (pred()) \
                        return; \
                    std::this_thread::sleep_for(std::chrono::nanoseconds(max_ns)); \
                } \
            } else { \
                if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) < max_ns) { \
                /*ASM_("nop; nop; nop; nop; nop; nop; nop"); */ \
                    if (pred()) \
                        return; \
                    do_pause<N, use_isb>(); \
                } else while (true) { \
                /*ASM_("nop; nop; nop; nop; nop; nop; nop; nop"); */ \
                    if (pred()) \
                        return; \
                    do_pause<max_ns / PAUSE_TIME, use_isb>(); /* should be max_ns */ \
                } \
            } \
        } /*else { \
            ASM_("nop; nop; nop; nop;"); \
        }*/


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
    PAUSE_AND_CHECK(1048576);
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
