// crill - the Cross-platform Real-time, I/O, and Low-Latency Library
// Copyright (c) 2022 - Timur Doumler and Fabian Renn-Giles
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

//
// Created by Timur Doumler on 06/02/2023.
//

#include <crill/seqlock.h>
#include <doctest/doctest.h>
#include <thread>

TEST_CASE("crill::seqlock")
{
    crill::seqlock sl;

    static_assert(!std::is_copy_constructible_v<crill::seqlock>);
    static_assert(!std::is_copy_assignable_v<crill::seqlock>);
    static_assert(!std::is_move_constructible_v<crill::seqlock>);
    static_assert(!std::is_move_assignable_v<crill::seqlock>);

    SUBCASE("Read operation")
    {
        bool read = false;

        sl.try_read([&] {
            read = true;
        });

        REQUIRE(read);
    }

    SUBCASE("Write operation")
    {
        bool write = false;

        sl.write([&] {
            write = true;
        });

        REQUIRE(write);
    }

    SUBCASE("Concurrent read/write: Writer does not wait if reader blocks")
    {
        std::atomic<bool> finish_reader = false;
        std::atomic<bool> reader_started = false;
        std::atomic<bool> reader_finished = false;
        bool write_succeeded = false;

        std::thread reader([&]{
            sl.try_read([&] {
                reader_started = true;
                while (!finish_reader) /* spin */;
                reader_finished = true;
            });
        });

        while (!reader_started) /* spin */;
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));

        sl.write([&] {
            write_succeeded = true;
        });

        REQUIRE(write_succeeded == true);
        REQUIRE(reader_finished == false);

        finish_reader = true;
        reader.join();
    }

    SUBCASE("Concurrent read/write: Reader returns false while writer blocks")
    {
        std::atomic<bool> finish_writer = false;
        std::atomic<bool> writer_started = false;
        std::atomic<bool> writer_finished = false;
        std::atomic<std::size_t> num_reads = 0;

        std::thread writer([&]{
            sl.write([&] {
                writer_started = true;
                while (!finish_writer) /* spin */;
                writer_finished = true;
            });
        });

        while (!writer_started) /* spin */;

        REQUIRE_FALSE(sl.try_read([&] { ++num_reads; }));
        // workaround
        auto num_reads_temp = num_reads.load();
        //REQUIRE(num_reads_temp == 1);

        finish_writer = true;
        while (!writer_finished) /* spin */;
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));

        REQUIRE(sl.try_read([&] { ++num_reads; }));
//        REQUIRE(num_reads == 2);

        writer.join();
    }

    SUBCASE("Concurrent reads do not block each other")
    {
        // TODO
    }
}

TEST_CASE("crill::seqlock_object")
{
    struct coeffs
    {
        std::size_t a = 0;
        bool b = false;
        std::size_t c = 42;
    };

    static_assert(std::is_trivially_copyable_v<coeffs>);
    static_assert(!std::atomic<coeffs>::is_always_lock_free);

    SUBCASE("load default-constructed instance")
    {
        crill::seqlock_object<coeffs> obj;

        coeffs c = obj.load();
        REQUIRE(c.a == 0);
        REQUIRE(c.b == false);
        REQUIRE(c.c == 42);
    }

    SUBCASE("try_load default-constructed instance")
    {
        crill::seqlock_object<coeffs> obj;

        coeffs c;
        REQUIRE(obj.try_load(c));
        REQUIRE(c.a == 0);
        REQUIRE(c.b == false);
        REQUIRE(c.c == 42);
    }

    SUBCASE("load")
    {
        crill::seqlock_object<coeffs> obj(coeffs{1, true, 2});

        auto coeffs = obj.load();
        REQUIRE(coeffs.a == 1);
        REQUIRE(coeffs.b == true);
        REQUIRE(coeffs.c == 2);
    }

    SUBCASE("try_load")
    {
        crill::seqlock_object<coeffs> obj(coeffs{1, true, 2});

        coeffs c;
        REQUIRE(obj.try_load(c));
        REQUIRE(c.a == 1);
        REQUIRE(c.b == true);
        REQUIRE(c.c == 2);
    }

    SUBCASE("store")
    {
        crill::seqlock_object<coeffs> obj;
        obj.store(coeffs{1, true, 2});

        coeffs c;
        REQUIRE(obj.try_load(c));
        REQUIRE(c.a == 1);
        REQUIRE(c.b == true);
        REQUIRE(c.c == 2);
    }

    SUBCASE("Concurrent load/store")
    {
        crill::seqlock_object<coeffs> obj;
        std::atomic<bool> writer_started = false;
        std::atomic<bool> stop = false;

        std::thread writer([&] {
            writer_started = true;
            std::size_t i = 0;
            while (!stop) {
                obj.store(coeffs{i, true, i});
                ++i;
            }
        });

        while (!writer_started)
            /* wait */;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        coeffs c = {};
        for (std::size_t i = 0; i < 1000; ++i)
            c = obj.load();

        REQUIRE(c.a > 0);
        REQUIRE(c.b == true);
        REQUIRE(c.c == c.a); // no torn writes

        stop = true;
        writer.join();
    }
}

TEST_CASE("crill::seqlock_object with size that is not a multiple of size_t")
{
    crill::seqlock_object<char> obj;
    obj.store('x');
    REQUIRE(obj.load() == 'x');
}
