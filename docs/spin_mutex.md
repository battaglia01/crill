# crill::spin_mutex Documentation

## Overview
The `crill::spin_mutex` is a spinlock with progressive backoff, designed for safely and efficiently synchronizing a real-time thread with other threads in an audio context. It is part of the crill library, which stands for Cross-platform Real-time, I/O, and Low-Latency Library.

The `crill::spin_mutex` is a drop-in replacement for `std::mutex`, meeting the standard C++ requirements for a mutex `[thread.req.lockable.req]`. It can be used with `std::scoped_lock` and `std::unique_lock` just like `std::mutex`.

The key difference between `crill::spin_mutex` and `std::mutex` is that `try_lock()` and `unlock()` in `crill::spin_mutex` are always wait-free, as they are implemented by setting a `std::atomic_flag`. In contrast, `std::mutex::unlock()` may perform a system call to wake up a waiting thread, which is not wait-free.

The `lock()` function in `crill::spin_mutex` is implemented with `crill::progressive_backoff_wait`, which uses a progressive backoff strategy to prevent wasting energy and allow other threads to progress while waiting for the lock.

The progressive backoff strategy has been tuned to provide comparable performance on both x64 and ARM architectures, making it suitable for cross-platform audio applications.

**Note**: `crill::spin_mutex` is not recursive; repeatedly locking it on the same thread is undefined behavior and will likely deadlock your application.

## Usage

```cpp
crill::spin_mutex mtx;

// Locking the mutex
mtx.lock();

// Unlocking the mutex
mtx.unlock();

// Trying to lock the mutex
if (mtx.try_lock()) {
    // Lock acquired
    // ...
    mtx.unlock();
} else {
    // Lock not acquired
    // ...
}

// Using with std::scoped_lock
{
    std::scoped_lock lock(mtx);
    // Mutex is locked within this scope
    // ...
} // Mutex is automatically unlocked when lock goes out of scope

// Using with std::unique_lock
std::unique_lock<crill::spin_mutex> lock(mtx);
// Mutex is locked
// ...
lock.unlock();
// Mutex is unlocked
```

## Public Interface

### Class: `spin_mutex`
**Purpose**: A spinlock with progressive backoff for safely and efficiently synchronizing a real-time thread with other threads.

- `lock()`
  - *Function*: Acquires the lock. If necessary, blocks until the lock can be acquired.
  - *Preconditions*: The current thread does not already hold the lock.
- `try_lock()`
  - *Function*: Attempts to acquire the lock without blocking.
  - *Returns*: `true` if the lock was acquired, `false` otherwise.
  - *Non-blocking guarantees*: wait-free.
- `unlock()`
  - *Function*: Releases the lock.
  - *Preconditions*: The lock is being held by the current thread.
  - *Non-blocking guarantees*: wait-free.

## Implementation Details

The `crill::spin_mutex` uses a `std::atomic_flag` to implement the lock. The `try_lock()` function attempts to set the flag using `test_and_set()` with `memory_order_acquire`, and returns `true` if successful (i.e., the flag was not previously set). The `unlock()` function clears the flag with `memory_order_release`.

The `lock()` function is implemented using `crill::progressive_backoff_wait`, which takes a predicate (in this case, `try_lock()`) and blocks the current thread until the predicate returns `true`. The blocking is implemented by spinning on the predicate with a progressive backoff strategy.

The progressive backoff strategy is implemented differently for x86, x86_64, and arm64 architectures:

- For x86 and x86_64 (Intel), it uses the `_mm_pause()` intrinsic to hint to the processor that the thread is in a spin-wait loop. The backoff strategy has three stages: a short spin, a longer spin with `_mm_pause()`, and an even longer spin with `_mm_pause()` followed by `std::this_thread::yield()` to give other threads a chance to progress.

- For arm64 (ARM 64-bit), it uses the `__wfe()` (wait for event) intrinsic to hint to the processor that the thread is waiting for an event. The backoff strategy has two stages: a short spin and a longer spin with `__wfe()` followed by `std::this_thread::yield()`.

The duration of each stage has been tuned to provide approximately 1 ms of total waiting time on typical 64-bit Intel and ARM machines before yielding to other threads.

## Performance Considerations

The `crill::spin_mutex` is designed to be fast and efficient for short critical sections, especially in the context of a real-time audio thread. The wait-free `try_lock()` and `unlock()` operations make it suitable for use in real-time contexts.

However, for longer critical sections or in cases of high contention, a spinlock may not be the most appropriate choice, as it can waste CPU cycles and potentially degrade the performance of other threads. In such cases, a standard mutex or other synchronization primitive may be more suitable.

The progressive backoff strategy helps to mitigate some of these issues by reducing the CPU load during spinning and allowing other threads to progress. Nevertheless, care should be taken to keep critical sections short and to choose the appropriate synchronization primitive for the specific use case.

## Limitations

- `crill::spin_mutex` is not recursive. Attempting to lock a mutex that is already owned by the current thread is undefined behavior and will likely result in a deadlock.
- The progressive backoff strategy is only implemented for x86, x86_64, and arm64 architectures. For other platforms, a preprocessor error will be generated.
- The tuning of the progressive backoff strategy is based on typical Intel and ARM machines and may not be optimal for all hardware configurations or usage patterns. Some experimentation may be required to find the best values for a specific use case.