// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
//
// This is a version of condition_variable which was written by Mike Battaglia
// and is not actually part of the crill library. It isn't much more than a
// wrapper around progressive_backoff_wait.

#ifndef CRILL_SPIN_CONDITION_VARIABLE_H
#define CRILL_SPIN_CONDITION_VARIABLE_H

#include <atomic>
#include <thread>
#include <functional>
#include <iostream>
#include <chrono>
#include <crill/progressive_backoff_wait.h>

namespace crill
{

// crill::spin_condition_variable is a mutex-free condition variable with progressive
// backoff for safely and efficiently synchronizing a real-time thread with other threads.
//
// crill::spin_condition_variable provides functionality similar to std::condition_variable,
// but without the need for a mutex. This is particularly useful in real-time applications
// where minimizing latency and avoiding blocking system calls is crucial.
//
// This implementation uses an integer counter to act like a semaphore, ensuring that
// no notifications are missed. Each notify() call increments the counter, and each
// successful wait() call decrements it. If multiple threads call notify() simultaneously,
// there may be a slight spin as wait() clears immediately once for each notify() call.
//
// wait() is implemented with crill::progressive_backoff_wait, to prevent wasting energy and
// allow other threads to progress.
//
// wait_for() and wait_until() provide timed waiting functionality.
//
// crill::spin_condition_variable is not designed for use cases that require a traditional
// condition variable with a mutex for complex waiting and notification patterns.
class spin_condition_variable
{
public:
    spin_condition_variable() : counter(0) {}

    // Effects: Blocks the current thread until the internal counter is greater than zero.
    // Blocking is implemented by spinning with a progressive backoff strategy.
    void wait()
    {
        progressive_backoff_wait([this] { return counter.load(std::memory_order_seq_cst) > 0; });
        counter.fetch_sub(1, std::memory_order_seq_cst); // Decrement the counter after successful wait
    }

    // Effects: Blocks the current thread until the predicate returns true.
    // Blocking is implemented by spinning on the predicate with a progressive backoff strategy.
    template <typename Predicate>
    void wait(Predicate&& pred)
    {
        progressive_backoff_wait(std::forward<Predicate>(pred));
    }

    // Effects: Blocks the current thread until the internal counter is greater than zero or the specified timeout duration has passed.
    template <typename Rep, typename Period>
    bool wait_for(const std::chrono::duration<Rep, Period>& timeout)
    {
        return wait_until(std::chrono::steady_clock::now() + timeout);
    }

    // Effects: Blocks the current thread until the predicate returns true or the specified timeout duration has passed.
    template <typename Predicate, typename Rep, typename Period>
    bool wait_for(Predicate&& pred, const std::chrono::duration<Rep, Period>& timeout)
    {
        return wait_until(std::forward<Predicate>(pred), std::chrono::steady_clock::now() + timeout);
    }

    // Effects: Blocks the current thread until the internal counter is greater than zero or the specified time point is reached.
    template <typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        bool timeout_reached = false;
        progressive_backoff_wait([this, &timeout_time, &timeout_reached] {
            if (Clock::now() >= timeout_time)
            {
                timeout_reached = true;
                return true;
            }
            return counter.load(std::memory_order_seq_cst) > 0;
        });
        if (!timeout_reached)
        {
            counter.fetch_sub(1, std::memory_order_seq_cst); // Decrement the counter after successful wait
        }
        return !timeout_reached;
    }

    // Effects: Blocks the current thread until the predicate returns true or the specified time point is reached.
    template <typename Predicate, typename Clock, typename Duration>
    bool wait_until(Predicate&& pred, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        bool timeout_reached = false;
        progressive_backoff_wait([&pred, &timeout_time, &timeout_reached] {
            if (Clock::now() >= timeout_time)
            {
                timeout_reached = true;
                return true;
            }
            return pred();
        });
        return !timeout_reached;
    }

    // Effects: Signals the condition, waking up one or more waiting threads.
    // This is a non-blocking operation and ensures memory visibility.
    void notify()
    {
        counter.fetch_add(1, std::memory_order_seq_cst);
    }

private:
    std::atomic<int> counter;
};

} // namespace crill

#endif // CRILL_SPIN_CONDITION_VARIABLE_H