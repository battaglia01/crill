// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
//
// This is modified by Mike Battaglia to be tuned for a condition variable rather
// than a spin lock.

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_CV_HYBRID_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_CV_HYBRID_H

#include <crill/platform.h>
#include <crill/impl/progressive_backoff_wait_impl_cv_hybrid.h>

namespace crill {

// Effects: Blocks the current thread until predicate returns true. Blocking is
// implemented by spinning on the predicate with a progressive backoff strategy.
//
// crill::progressive_backoff_wait is used to implement crill::spin_mutex, but is also
// useful on its own, in scenarios where a thread needs to wait on something else
// than a spinlock being released (for example, a CAS loop).
//
// Compared to a naive implementation like
//
//    while (!predicate()) /* spin */;
//
// the progressive backoff strategy prevents wasting energy, and allows other threads
// to progress by yielding from the waiting thread after a certain amount of time.
// This time is currently chosen to be approximately 1 ms on a typical 64-bit Intel
// or ARM based machine.
//
// On platforms other than x86, x86_64, and arm64, no implementation is currently available.
template <typename Predicate>
void progressive_backoff_wait_cv_hybrid(Predicate&& pred)
{
  #if CRILL_INTEL
    impl::progressive_backoff_wait_intel_cv_hybrid <5, 10, 50, 20, 5>(std::forward<Predicate>(pred));
    // approx:
    // - 5x5 ns (= 25 ns), 0 pauses per loop
    // - 10x40 ns (= 400 ns), 1 pause per loop
    // - 50x350 ns (17.5 µs), 10 pauses (extrapolated from original benchmark)
    // - 20x17.5 µs (350 µs), 500 pauses (time estimated as above)
    // - 5x1ms (~ 1 ms), sleeping the thread for 1ms per loop
    // - then sleeping for 5ms per loop
    // these are extrapolated from Timur's original benchmarks on a 2.9 GHz Intel i9
  #elif CRILL_ARM_64BIT
    impl::progressive_backoff_wait_armv8_cv_hybrid<2, 10, 25, 5>(std::forward<Predicate>(pred));
    // approx:
    // - 2x10 ns (= 20 ns), 0 pauses per loop
    // - 10x1.333 µs (~ 13.33 µs), 1 pause per loop (extrapolated from original benchmark)
    // - 25x13.33 µs (~ 333.25 µs), 10 pauses (time estimated as above)
    // - 5x1ms (~ 1 ms), sleeping the thread for 1ms per loop
    // - then sleeping for 5ms per loop
    // these are extrapolated from Timur's original benchmarks on an Apple
    // Silicon Mac/armv8 based phone.
  #else
    #error "Platform not supported!"
  #endif
}

} // namespace crill

#endif // CRILL_PROGRESSIVE_BACKOFF_WAIT_H
