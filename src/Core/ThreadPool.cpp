#include "Core/ThreadPool.hpp"
#include <mutex>

ThreadPool::ThreadPool(size_t num_threads)
{
    for (size_t i = 0; i < num_threads; i++)
    {
        m_threads.emplace_back(&ThreadPool::thread_worker, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_stop = true;
    }

    m_cv.notify_all();

    for (auto& thread : m_threads)
    {
        thread.join();
    }
}

Result<void> ThreadPool::async(std::function<void()> task)
{
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        TRY(m_tasks.append(task));
    }

    m_cv.notify_one();
    return Result<void>();
}

void ThreadPool::thread_worker()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_cv.wait(lock, [this]
                      { return !m_tasks.empty() || m_stop; });

            if (m_stop && m_tasks.empty())
            {
                return;
            }

            task = std::move(m_tasks.pop_front_unchecked());
        }

        task();
    }
}
