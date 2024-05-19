#ifndef CRILL_spin_condition_variable_pure_exp_H
#define CRILL_spin_condition_variable_pure_exp_H

#include <atomic>
#include <thread>
#include <functional>
#include <iostream>
#include <chrono>
#include <crill/progressive_backoff_wait_pure_exp.h>

namespace crill
{

// crill::spin_condition_variable_pure_exp is a mutex-free condition variable with progressive
// backoff for safely and efficiently synchronizing a real-time thread with other threads.
//
// crill::spin_condition_variable_pure_exp provides functionality similar to std::condition_variable,
// but without the need for a mutex. This is particularly useful in real-time applications
// where minimizing latency and avoiding blocking system calls is crucial.
//
// This implementation uses an atomic flag with compare-and-set to ensure that the flag
// is checked and cleared atomically, avoiding race conditions.
//
// wait() is implemented with crill::progressive_backoff_wait_cv, to prevent wasting energy and
// allow other threads to progress.
//
// wait_for() and wait_until() provide timed waiting functionality.
//
// crill::spin_condition_variable_pure_exp is not designed for use cases that require a traditional
// condition variable with a mutex for complex waiting and notification patterns.
template <unsigned long long max_ns, unsigned long long sleep_threshold_ns>
class spin_condition_variable_pure_exp
{
public:
    spin_condition_variable_pure_exp() : flag(false) {}

    // Effects: Blocks the current thread until the internal flag is set to true.
    // Blocking is implemented by spinning with a progressive backoff strategy.
    void wait()
    {
        progressive_backoff_wait_pure_exp<max_ns, sleep_threshold_ns>([this] {
            bool expected = true;
            return flag.compare_exchange_strong(expected, false, std::memory_order_seq_cst);
        });
    }

    // Effects: Blocks the current thread until the predicate returns true.
    // Blocking is implemented by spinning on the predicate with a progressive backoff strategy.
    template <typename Predicate>
    void wait(Predicate&& pred)
    {
        progressive_backoff_wait_pure_exp<max_ns, sleep_threshold_ns>(std::forward<Predicate>(pred));
    }

    // Effects: Blocks the current thread until the internal flag is set to true or the specified timeout duration has passed.
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

    // Effects: Blocks the current thread until the internal flag is set to true or the specified time point is reached.
    template <typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        bool timeout_reached = false;
        progressive_backoff_wait_pure_exp<max_ns, sleep_threshold_ns>([this, &timeout_time, &timeout_reached] {
            if (Clock::now() >= timeout_time)
            {
                timeout_reached = true;
                return true;
            }
            bool expected = true;
            return flag.compare_exchange_strong(expected, false, std::memory_order_seq_cst);
        });
        return !timeout_reached;
    }

    // Effects: Blocks the current thread until the predicate returns true or the specified time point is reached.
    template <typename Predicate, typename Clock, typename Duration>
    bool wait_until(Predicate&& pred, const std::chrono::time_point<Clock, Duration>& timeout_time)
    {
        bool timeout_reached = false;
        progressive_backoff_wait_pure_exp<max_ns, sleep_threshold_ns>([&pred, &timeout_time, &timeout_reached] {
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
        flag.store(true, std::memory_order_seq_cst);
    }

private:
    std::atomic<bool> flag;
};

} // namespace crill
#endif //CRILL_spin_condition_variable_pure_exp_H