// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
//
// This is modified by Mike Battaglia to be tuned for a condition variable rather
// than a spin lock.

#ifndef CRILL_PROGRESSIVE_BACKOFF_WAIT_PURE_EXP_H
#define CRILL_PROGRESSIVE_BACKOFF_WAIT_PURE_EXP_H

#include <crill/platform.h>
#include <crill/impl/progressive_backoff_wait_impl_pure_exp.h>

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
template <unsigned long long min_ns, unsigned long long max_ns, unsigned long long sleep_threshold_ns, bool use_isb, typename Predicate>
void progressive_backoff_wait_pure_exp(Predicate&& pred)
{
    impl::progressive_backoff_wait_pure_exp<min_ns, max_ns, sleep_threshold_ns, use_isb>(std::forward<Predicate>(pred));
}

} // namespace crill

#endif // CRILL_PROGRESSIVE_BACKOFF_WAIT_H
