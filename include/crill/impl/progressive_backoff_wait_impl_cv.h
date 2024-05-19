// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
//
// This is modified by Mike Battaglia to be tuned for a condition variable rather
// than a spin lock.

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_CV_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_CV_H

#define REPEAT_5_TIMES(x) x; x; x; x; x; 
#define REPEAT_10_TIMES(x) REPEAT_5_TIMES(x) REPEAT_5_TIMES(x)
#define REPEAT_50_TIMES(x) REPEAT_10_TIMES(REPEAT_5_TIMES(x))
#define REPEAT_100_TIMES(x) REPEAT_10_TIMES(REPEAT_10_TIMES(x))
#define REPEAT_500_TIMES(x) REPEAT_100_TIMES(REPEAT_5_TIMES(x))
#define REPEAT_1000_TIMES(x) REPEAT_100_TIMES(REPEAT_10_TIMES(x))
#define REPEAT_5000_TIMES(x) REPEAT_1000_TIMES(REPEAT_5_TIMES(x))
#define REPEAT_10000_TIMES(x) REPEAT_1000_TIMES(REPEAT_10_TIMES(x))

#include <thread>

#if CRILL_INTEL
  #include <emmintrin.h>
#elif CRILL_ARM_64BIT
  #include <arm_acle.h>
#endif

namespace crill::impl
{
  #if CRILL_INTEL
    template <std::size_t N0, std::size_t N1, std::size_t N2, std::size_t N3, std::size_t N4, typename Predicate>
    void progressive_backoff_wait_intel_cv(Predicate&& pred)
    {
        // phase 1
        for (int i = 0; i < N0; ++i)
        {
            if (pred())
                return;
        }

        // phase 2
        for (int i = 0; i < N1; ++i)
        {
            if (pred())
                return;

            _mm_pause();
        }

        // phase 3
        for (int i = 0; i < N2; ++i)
        {
            if (pred())
                return;

            REPEAT_10_TIMES(_mm_pause());
        }

        // phase 4
        for (int i = 0; i < N3; ++i)
        {
            if (pred())
                return;

            REPEAT_500_TIMES(_mm_pause());
        }

        // phase 5
        while (true) {
            for (int i = 0; i < N4; ++i)
            {
                if (pred())
                    return;

                REPEAT_10000_TIMES(_mm_pause());
            }

            std::this_thread::yield();
        }
    }
  #endif // CRILL_INTEL

  #if CRILL_ARM_64BIT
    template <std::size_t N0, std::size_t N1, std::size_t N2, std::size_t N3, typename Predicate>
    void progressive_backoff_wait_armv8_cv(Predicate&& pred)
    {
        // phase 1
        for (int i = 0; i < N0; ++i)
        {
            if (pred())
                return;
        }

        // phase 2
        for (int i = 0; i < N1; ++i)
        {
            if (pred())
                return;

            __wfe();
        }

        // phase 3
        for (int i = 0; i < N2; ++i)
        {
            if (pred())
                return;

            REPEAT_10_TIMES(__wfe());
        }

        // phase 4
        while (true) {
            for (int i = 0; i < N3; ++i)
            {
                if (pred())
                    return;

                REPEAT_100_TIMES(__wfe());
                REPEAT_100_TIMES(__wfe());
                REPEAT_50_TIMES(__wfe());   
            }
            std::this_thread::yield();
        }
    }
  #endif // CRILL_ARM_64BIT
} // namespace crill::impl

#endif //CRILL_PROGRESSIVE_BACKOFF_WAIT_IMPL_H
