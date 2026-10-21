#pragma once

#include "daking/MPSC_queue.hpp"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    using TaskFn = std::function<void(std::stop_token)>;

    ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Only consumes this calling thread's wait time, across all pools.
    static double take_submit_wait_ms();

    size_t size() const { return m_threads.size(); }

    /// Starts an asynchronous task.
    void submit(TaskFn task);

    /// Cancel pending work and join running tasks. Call from the owning thread.
    void stop();
    /// Stop old work, then start fresh workers for a new world.
    void reset();

private:
    std::mutex m_mutex;
    // std::deque<std::function<void(std::stop_token)>> m_tasks;

    daking::MPSC_queue<TaskFn> m_tasks;

    std::condition_variable m_cv;
    bool m_should_stop = false;

    // Thread must be destroyed before mutex.
    std::vector<std::jthread> m_threads;

    void thread_worker(std::stop_token token);
    void start(size_t num_threads);
};
