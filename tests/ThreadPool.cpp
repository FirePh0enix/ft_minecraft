// Standalone: c++ -std=c++23 -pthread -Isrc -Isrc/Core -Ibuild/_deps/mpsc_queue-src/include tests/ThreadPool.cpp src/Core/ThreadPool.cpp -o /tmp/thread-pool-test
#include "Core/ThreadPool.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <future>

int main()
{
    using namespace std::chrono_literals;
    ThreadPool pool(2);
    for (int world = 0; world < 20; ++world)
    {
        std::atomic<int> finished = 0;
        std::atomic<int> stale = 0;
        std::promise<void> started[2];
        for (int i = 0; i < 2; ++i)
            pool.submit([&, i](std::stop_token token)
            {
                started[i].set_value();
                while (!token.stop_requested())
                    std::this_thread::yield();
                ++finished;
            });
        for (auto& signal : started)
            assert(signal.get_future().wait_for(5s) == std::future_status::ready);

        // Both workers are busy: these old-world jobs must be discarded.
        for (int i = 0; i < 100; ++i)
            pool.submit([&](std::stop_token) { ++stale; });
        pool.reset();
        assert(finished == 2);
        assert(stale == 0);
        assert(pool.size() == 2);

        std::promise<void> fresh;
        pool.submit([&](std::stop_token token)
        {
            assert(!token.stop_requested());
            fresh.set_value();
        });
        assert(fresh.get_future().wait_for(5s) == std::future_status::ready);
        // Join before destroying anything captured by the fresh task.
        pool.reset();
    }
    pool.stop();
    pool.stop();
    pool.submit([](std::stop_token) { assert(false); });
}
