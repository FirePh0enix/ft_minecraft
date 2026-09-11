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

    size_t size() const { return m_threads.size(); }

    /// Starts an asynchronous task.
    void submit(TaskFn task);

private:
    std::mutex m_mutex;
    // std::deque<std::function<void(std::stop_token)>> m_tasks;

    daking::MPSC_queue<TaskFn> m_tasks;

    std::condition_variable m_cv;
    bool m_should_stop = false;

    // Thread must be destroyed before mutex.
    std::vector<std::jthread> m_threads;

    void thread_worker(std::stop_token token);
};
