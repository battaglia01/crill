// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_PURE_EXP_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_PURE_EXP_H

#include <thread>
#include <cmath>
#include <iostream>

#if CRILL_INTEL
    #include <emmintrin.h>
    constexpr unsigned long long PAUSE_TIME = 35; // ns, as benchmarked by Timur
    #define PAUSE() _mm_pause();
#elif CRILL_ARM_64BIT
    #include <arm_acle.h>
    constexpr unsigned long long PAUSE_TIME = 1333; // ns, as benchmarked by Timur
    #define PAUSE() __wfe();
#else
    #error "Platform not supported!"
#endif

// #define REPEAT_1_TIMES(x) x
// #define REPEAT_2_TIMES(x) REPEAT_1_TIMES(x) REPEAT_1_TIMES(x)
// #define REPEAT_4_TIMES(x) REPEAT_2_TIMES(x) REPEAT_2_TIMES(x)
// #define REPEAT_8_TIMES(x) REPEAT_4_TIMES(x) REPEAT_4_TIMES(x)
// #define REPEAT_16_TIMES(x) REPEAT_8_TIMES(x) REPEAT_8_TIMES(x)
// #define REPEAT_32_TIMES(x) for (unsigned long long i = 0; i < 2; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_64_TIMES(x) for (unsigned long long i = 0; i < 4; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_128_TIMES(x) for (unsigned long long i = 0; i < 8; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_256_TIMES(x) for (unsigned long long i = 0; i < 16; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_512_TIMES(x) for (unsigned long long i = 0; i < 32; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_1024_TIMES(x) for (unsigned long long i = 0; i < 64; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_2048_TIMES(x) for (unsigned long long i = 0; i < 128; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_4096_TIMES(x) for (unsigned long long i = 0; i < 256; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_8192_TIMES(x) for (unsigned long long i = 0; i < 512; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_16384_TIMES(x) for (unsigned long long i = 0; i < 1024; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_32768_TIMES(x) for (unsigned long long i = 0; i < 2048; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_65536_TIMES(x) for (unsigned long long i = 0; i < 4096; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_131072_TIMES(x) for (unsigned long long i = 0; i < 8192; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_262144_TIMES(x) for (unsigned long long i = 0; i < 16384; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_524288_TIMES(x) for (unsigned long long i = 0; i < 32768; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_1048576_TIMES(x) for (unsigned long long i = 0; i < 65536; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_2097152_TIMES(x) for (unsigned long long i = 0; i < 131072; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_4194304_TIMES(x) for (unsigned long long i = 0; i < 262144; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_8388608_TIMES(x) for (unsigned long long i = 0; i < 524288; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_16777216_TIMES(x) for (unsigned long long i = 0; i < 1048576; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_33554432_TIMES(x) for (unsigned long long i = 0; i < 2097152; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_67108864_TIMES(x) for (unsigned long long i = 0; i < 4194304; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_134217728_TIMES(x) for (unsigned long long i = 0; i < 8388608; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_268435456_TIMES(x) for (unsigned long long i = 0; i < 16777216; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_536870912_TIMES(x) for (unsigned long long i = 0; i < 33554432; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_1073741824_TIMES(x) for (unsigned long long i = 0; i < 67108864; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_2147483648_TIMES(x) for (unsigned long long i = 0; i < 134217728; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_4294967296_TIMES(x) for (unsigned long long i = 0; i < 268435456; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_8589934592_TIMES(x) for (unsigned long long i = 0; i < 536870912; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_17179869184_TIMES(x) for (unsigned long long i = 0; i < 1073741824; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_34359738368_TIMES(x) for (unsigned long long i = 0; i < 2147483648; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_68719476736_TIMES(x) for (unsigned long long i = 0; i < 4294967296; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_137438953472_TIMES(x) for (unsigned long long i = 0; i < 8589934592; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_274877906944_TIMES(x) for (unsigned long long i = 0; i < 17179869184; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_549755813888_TIMES(x) for (unsigned long long i = 0; i < 34359738368; ++i) { REPEAT_16_TIMES(x) }
// #define REPEAT_1099511627776_TIMES(x) for (unsigned long long i = 0; i < 68719476736; ++i) { REPEAT_16_TIMES(x) }

#define REPEAT_1_TIMES(x) x;
#define REPEAT_2_TIMES(x) x; x;
#define REPEAT_4_TIMES(x) REPEAT_2_TIMES(x) REPEAT_2_TIMES(x)
#define REPEAT_8_TIMES(x) REPEAT_4_TIMES(x) REPEAT_4_TIMES(x)
#define REPEAT_16_TIMES(x) REPEAT_8_TIMES(x) REPEAT_8_TIMES(x)
#define REPEAT_32_TIMES(x) REPEAT_16_TIMES(x) REPEAT_16_TIMES(x)
#define REPEAT_64_TIMES(x) REPEAT_32_TIMES(x) REPEAT_32_TIMES(x)
#define REPEAT_128_TIMES(x) REPEAT_64_TIMES(x) REPEAT_64_TIMES(x)
#define REPEAT_256_TIMES(x) REPEAT_128_TIMES(x) REPEAT_128_TIMES(x)
#define REPEAT_512_TIMES(x) REPEAT_256_TIMES(x) REPEAT_256_TIMES(x)
#define REPEAT_1024_TIMES(x) REPEAT_512_TIMES(x) REPEAT_512_TIMES(x)
#define REPEAT_2048_TIMES(x) REPEAT_1024_TIMES(x) REPEAT_1024_TIMES(x)
#define REPEAT_4096_TIMES(x) REPEAT_2048_TIMES(x) REPEAT_2048_TIMES(x)
#define REPEAT_8192_TIMES(x) REPEAT_4096_TIMES(x) REPEAT_4096_TIMES(x)
#define REPEAT_16384_TIMES(x) REPEAT_8192_TIMES(x) REPEAT_8192_TIMES(x)
#define REPEAT_32768_TIMES(x) REPEAT_16384_TIMES(x) REPEAT_16384_TIMES(x)
#define REPEAT_65536_TIMES(x) REPEAT_32768_TIMES(x) REPEAT_32768_TIMES(x)
#define REPEAT_131072_TIMES(x) REPEAT_65536_TIMES(x) REPEAT_65536_TIMES(x)
#define REPEAT_262144_TIMES(x) for (unsigned long long i = 0; i < 2; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_524288_TIMES(x) for (unsigned long long i = 0; i < 4; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_1048576_TIMES(x) for (unsigned long long i = 0; i < 8; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_2097152_TIMES(x) for (unsigned long long i = 0; i < 16; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_4194304_TIMES(x) for (unsigned long long i = 0; i < 32; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_8388608_TIMES(x) for (unsigned long long i = 0; i < 64; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_16777216_TIMES(x) for (unsigned long long i = 0; i < 128; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_33554432_TIMES(x) for (unsigned long long i = 0; i < 256; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_67108864_TIMES(x) for (unsigned long long i = 0; i < 512; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_134217728_TIMES(x) for (unsigned long long i = 0; i < 1024; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_268435456_TIMES(x) for (unsigned long long i = 0; i < 2048; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_536870912_TIMES(x) for (unsigned long long i = 0; i < 4096; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_1073741824_TIMES(x) for (unsigned long long i = 0; i < 8192; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_2147483648_TIMES(x) for (unsigned long long i = 0; i < 16384; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_4294967296_TIMES(x) for (unsigned long long i = 0; i < 32768; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_8589934592_TIMES(x) for (unsigned long long i = 0; i < 65536; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_17179869184_TIMES(x) for (unsigned long long i = 0; i < 131072; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_34359738368_TIMES(x) for (unsigned long long i = 0; i < 262144; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_68719476736_TIMES(x) for (unsigned long long i = 0; i < 524288; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_137438953472_TIMES(x) for (unsigned long long i = 0; i < 1048576; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_274877906944_TIMES(x) for (unsigned long long i = 0; i < 2097152; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_549755813888_TIMES(x) for (unsigned long long i = 0; i < 4194304; ++i) { REPEAT_131072_TIMES(x) }
#define REPEAT_1099511627776_TIMES(x) for (unsigned long long i = 0; i < 8388608; ++i) { REPEAT_131072_TIMES(x) }

#define PAUSE_AND_CHECK(N) \
    if constexpr ((PAUSE_TIME * static_cast<unsigned long long>(N)) >= min_ns) { \
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
                REPEAT_##N##_TIMES(PAUSE()); \
            } else while (true) { \
                if (pred()) \
                    return; \
                REPEAT_##N##_TIMES(PAUSE()); \
            } \
        } \
    }


namespace crill::impl {

template <unsigned long long min_ns, unsigned long long max_ns, unsigned long long sleep_threshold_ns, typename Predicate>
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
