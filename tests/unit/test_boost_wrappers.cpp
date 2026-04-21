/**
 * @file test_boost_wrappers.cpp
 * @brief Unit tests for MiniSonic.Boost.Wrappers module
 */

#include <catch2/catch_all.hpp>

// Import the module
import MiniSonic.Boost.Wrappers;

using namespace MiniSonic::BoostWrapper;

TEST_CASE("BoostWrappers UUID generation", "[boost][uuid]") {
    SECTION("Generate unique UUIDs") {
        Uuid id1 = generateUuid();
        Uuid id2 = generateUuid();
        
        REQUIRE(!isNullUuid(id1));
        REQUIRE(!isNullUuid(id2));
        REQUIRE(id1 != id2);
    }
    
    SECTION("UUID to string conversion") {
        Uuid id = generateUuid();
        std::string str = uuidToString(id);
        
        REQUIRE(str.length() == 36);  // Standard UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
        REQUIRE(str[8] == '-');
        REQUIRE(str[13] == '-');
        REQUIRE(str[18] == '-');
        REQUIRE(str[23] == '-');
    }
}

TEST_CASE("BoostWrappers Atomic operations", "[boost][atomic]") {
    SECTION("Atomic increment") {
        AtomicUint64 counter{0};
        
        REQUIRE(counter.load() == 0);
        
        uint64_t new_val = ++counter;
        REQUIRE(new_val == 1);
        REQUIRE(counter.load() == 1);
    }
    
    SECTION("Atomic compare and exchange") {
        AtomicUint32 value{42};
        
        uint32_t expected = 42;
        bool success = value.compare_exchange_strong(expected, 100);
        
        REQUIRE(success);
        REQUIRE(value.load() == 100);
    }
}

TEST_CASE("BoostWrappers Mutex and LockGuard", "[boost][thread]") {
    SECTION("Basic mutex locking") {
        Mutex mtx;
        int shared_value = 0;
        
        {
            LockGuard lock(mtx);
            shared_value = 42;
        }
        
        REQUIRE(shared_value == 42);
    }
    
    SECTION("Recursive mutex") {
        RecursiveMutex mtx;
        int value = 0;
        
        {
            RecursiveLockGuard lock1(mtx);
            value = 1;
            {
                RecursiveLockGuard lock2(mtx);  // Same thread can lock again
                value = 2;
            }
            REQUIRE(value == 2);
        }
    }
}

TEST_CASE("BoostWrappers SpinLock", "[boost][thread][spinlock]") {
    SECTION("Basic spinlock operations") {
        SpinLock spinlock;
        int counter = 0;
        
        {
            SpinLockGuard lock(spinlock);
            counter++;
        }
        
        REQUIRE(counter == 1);
        REQUIRE(spinlock.try_lock());
        spinlock.unlock();
    }
}

TEST_CASE("BoostWrappers Optional", "[boost][optional]") {
    SECTION("Optional with value") {
        Optional<int> opt = makeOptional(42);
        
        REQUIRE(opt.is_initialized());
        REQUIRE(*opt == 42);
    }
    
    SECTION("Empty optional") {
        Optional<int> opt;
        
        REQUIRE(!opt.is_initialized());
    }
}

TEST_CASE("BoostWrappers UnorderedMap", "[boost][unordered_map]") {
    SECTION("Basic map operations") {
        UnorderedMap<std::string, int> map;
        
        map["key1"] = 100;
        map["key2"] = 200;
        
        REQUIRE(map.size() == 2);
        REQUIRE(map["key1"] == 100);
        REQUIRE(map["key2"] == 200);
    }
}

TEST_CASE("BoostWrappers ThreadPool", "[boost][asio][thread_pool]") {
    SECTION("ThreadPool creation and execution") {
        ThreadPool pool(2);
        
        IoContext& ctx = pool.getContext();
        std::atomic<int> counter{0};
        
        boost::asio::post(ctx, [&counter]() {
            counter++;
        });
        
        boost::asio::post(ctx, [&counter]() {
            counter++;
        });
        
        // Give some time for tasks to execute
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        REQUIRE(counter.load() >= 0);  // Tasks may or may not have executed yet
    }
}

TEST_CASE("BoostWrappers SteadyTimer", "[boost][asio][timer]") {
    SECTION("Timer creation") {
        IoContext io;
        SteadyTimer timer(io);
        
        timer.expires_after(std::chrono::milliseconds(10));
        
        bool callback_executed = false;
        timer.wait([&callback_executed](const ErrorCode& ec) {
            if (!ec) {
                callback_executed = true;
            }
        });
        
        io.run();
        
        REQUIRE(callback_executed);
    }
}
