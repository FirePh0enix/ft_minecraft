#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency() - 1);
    ~ThreadPool();

    /**
     * Starts an asynchronous task.
     * TODO: Should returns a future that can be canceled.
     */
    void async(std::function<void()> task);

private:
    std::vector<std::thread> m_threads;

    std::mutex m_queue_mutex;
    std::vector<std::function<void()>> m_tasks;

    std::condition_variable m_cv;
    bool m_stop = false;

    void thread_worker();
};
