// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

#ifndef CRILL_RECLAIM_OBJECT_TL_H
#define CRILL_RECLAIM_OBJECT_TL_H

#include <iostream>
#include <cassert>
#include <utility>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>
#include <crill/atomic_unique_ptr.h>

namespace crill
{

// crill::reclaim_object_tl stores a value of type T and provides concurrent
// read and write access to it. Multiple readers and writers are supported.
// 
// In this version, readers are automatically generated for each thread. The
// maximum number of threads is specified by the template parameter
// max_num_threads, which defaults to 128.
//
// Readers are guaranteed to always be wait-free. Readers will never block
// writers, but writers may block other writers.
//
// Overwritten values are put on a "zombie list". Values on the zombie list
// that are no longer referred to by any reader can be reclaimed by calling
// reclaim(). A call to reclaim() will block writers.
//
// The principle is very similar to RCU, with two key differences:
// 1) reclamation is managed per object, not in a single global domain
// 2) reclamation does not happen automatically: the user needs to explicitly
//    call reclaim() periodically (e.g., on a timer).
template <typename T, size_t max_num_threads=128>
class reclaim_object_tl
{
public:
    // Effects: constructs a reclaim_object_tl containing a default-constructed value.
    reclaim_object_tl()
      : value(std::make_unique<T>())
    {
        init_thread_readers();
    }

    // Effects: constructs a reclaim_object_tl containing a value constructed with
    // the constructor arguments provided.
    template <typename... Args>
    reclaim_object_tl(Args&&... args)
      : value(std::make_unique<T>(std::forward<Args>(args)...))
    {
        init_thread_readers();
    }

    void init_thread_readers()
    {
        thread_readers.reserve(max_num_threads);
        for (size_t i = 0; i < max_num_threads; ++i)
        {
            thread_readers.emplace_back(*this);
        }
    }

    // reclaim_object_tl is non-copyable and non-movable.
    reclaim_object_tl(reclaim_object_tl&&) = delete;
    reclaim_object_tl& operator=(reclaim_object_tl&&) = delete;
    reclaim_object_tl(const reclaim_object_tl&) = delete;
    reclaim_object_tl& operator=(const reclaim_object_tl&) = delete;

    // Reading the value must happen through a reader class.
    class reader;

    // read_ptr provides scoped read access to the value.
    class read_ptr
    {
    public:
        read_ptr(reader& rdr) noexcept
          : rdr(rdr)
        {
            rdr.num_reading++;
            if (rdr.min_epoch == 0)
            {
                // This is the first time we've gotten the lock
                rdr.min_epoch.store(rdr.obj.current_epoch.load());
                assert(rdr.min_epoch != 0);

                rdr.value_read = rdr.obj.value.load();
                assert(rdr.value_read);
            }
        }

        ~read_ptr()
        {
            assert(rdr.min_epoch != 0);
            assert(rdr.num_reading > 0);
            if (--rdr.num_reading == 0) {
                // This is the last reader to release the lock
                rdr.value_read = nullptr;
                rdr.min_epoch.store(0);
            }
        }

        const T& operator*() const
        {
            assert(rdr.value_read);
            return *rdr.value_read;
        }

        const T* operator->() const
        {
            assert(rdr.value_read);
            return rdr.value_read;
        }

        read_ptr(read_ptr&&) = delete;
        read_ptr& operator=(read_ptr&&) = delete;
        read_ptr(const read_ptr&) = delete;
        read_ptr& operator=(const read_ptr&) = delete;

    private:
        reader& rdr;
    };

    class reader
    {
    public:
        reader(reclaim_object_tl& obj) : obj(obj)
        {
            // no need to register or unregister thread readers as they're
            // pre-allocated in the constructor of reclaim_object_tl
        }

        ~reader()
        {
        }

        // make move constructor that isn't delete
        reader(reader&& other): obj(other.obj), min_epoch(other.min_epoch.load()), value_read(other.value_read), num_reading(other.num_reading)
        {
        }


        // Returns: a copy of the current value.
        // Non-blocking guarantees: wait-free if the copy constructor of
        // T is wait-free.
        T get_value() noexcept(std::is_nothrow_copy_constructible_v<T>)
        {
            return *read_lock();
        }

        // Returns: a read_ptr giving read access to the current value.
        // Non-blocking guarantees: wait-free.
        read_ptr read_lock() noexcept
        {
            return read_ptr(*this);
        }

    private:
        friend class reclaim_object_tl;
        friend class read_ptr;

        reclaim_object_tl& obj;
        std::atomic<std::uint64_t> min_epoch = 0;
        T *value_read = nullptr; // goes here for reentrancy
        int num_reading = 0;
    };

    reader& get_reader()
    {
        // This is a process-wide counter. Its purpose is to assign a unique
        // low-numbered ID to each thread that requests a reader. This ID is
        // used to index into the thread_readers vector.
        static std::atomic<size_t> thread_counter = 0;

        // Initialize thread_id once per thread using a lambda function.
        thread_local size_t thread_id = []() -> size_t {
            size_t id = thread_counter.fetch_add(1);
            if (id >= max_num_threads) {
                throw std::runtime_error("Exceeded maximum number of supported threads.");
            }
            return id;
        }();
        auto tmp_thread_id = thread_id;
        return thread_readers.at(tmp_thread_id);
    }

    read_ptr read_lock()
    {
        return get_reader().read_lock();
    }

    // Effects: Updates the current value to a new value constructed from the
    // provided constructor arguments.
    // Note: allocates memory.
    template <typename... Args>
    void update(Args&&... args)
    {
        exchange_and_retire(std::make_unique<T>(std::forward<Args>(args)...));
    }

    // write_ptr provides scoped write access to the value. This is useful if
    // you want to modify e.g. only a single data member of a larger class.
    // The new value will be atomically published when write_ptr goes out of scope.
    class write_ptr
    {
    public:
        write_ptr(reclaim_object_tl& obj, bool reclaim_on_write)
          : obj(obj),
            reclaim_on_write(reclaim_on_write),
            new_value(std::make_unique<T>(*obj.value.load()))
        {
            assert(new_value);
        }

        ~write_ptr()
        {
            assert(new_value);
            obj.exchange_and_retire(std::move(new_value));

            if (reclaim_on_write)
                obj.reclaim();
        }

        T& operator*()
        {
            assert(new_value);
            return *new_value;
        }

        T* operator->() noexcept
        {
            assert(new_value);
            return new_value.get();
        }

        write_ptr(write_ptr&&) = delete;
        write_ptr& operator=(write_ptr&&) = delete;
        write_ptr(const write_ptr&) = delete;
        write_ptr& operator=(const write_ptr&) = delete;

    private:
        reclaim_object_tl& obj;
        std::unique_ptr<T> new_value;
        bool reclaim_on_write;
    };

    // Returns: a write_ptr giving scoped write access to the current value.
    write_ptr write_lock()
    {
        return write_ptr(*this, false);
    }

    write_ptr write_and_reclaim_lock()
    {
        return write_ptr(*this, true);
    }

    // Effects: Deletes all previously overwritten values that are no longer
    // referred to by a read_ptr.
    void reclaim()
    {
        std::scoped_lock lock(zombies_mtx);

        for (auto& zombie : zombies)
        {
            assert(zombie.value != nullptr);
            if (!has_readers_using_epoch(zombie.epoch_when_retired))
                zombie.value.reset();
        }

        zombies.erase(
            std::remove_if(zombies.begin(), zombies.end(), [](auto& zombie){ return zombie.value == nullptr; }),
            zombies.end());
    }

private:
    void exchange_and_retire(std::unique_ptr<T> new_value)
    {
        assert(new_value != nullptr);
        auto old_value = value.exchange(std::move(new_value));

        std::scoped_lock lock(zombies_mtx);
        zombies.push_back({
            current_epoch.fetch_add(1),
            std::move(old_value)});
    }

    bool has_readers_using_epoch(std::uint64_t epoch) noexcept
    {
        return std::any_of(thread_readers.begin(), thread_readers.end(), [epoch](auto& reader) {
            std::uint64_t reader_epoch = reader.min_epoch.load();
            return reader_epoch != 0 && reader_epoch <= epoch;
        });
    }

    struct zombie
    {
        std::uint64_t epoch_when_retired;
        std::unique_ptr<T> value;
    };

    crill::atomic_unique_ptr<T> value;
    std::vector<reader> thread_readers;
    std::vector<zombie> zombies;
    std::mutex zombies_mtx;
    std::atomic<std::uint64_t> current_epoch = 1;

    // This algorithm requires a 64-bit lock-free atomic counter to avoid overflow.
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free);
};

} // namespace crill

#endif //CRILL_RECLAIM_OBJECT_TL_H
