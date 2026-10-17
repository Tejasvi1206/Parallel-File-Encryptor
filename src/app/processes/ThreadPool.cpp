#include <iostream>
#include "ThreadPool.hpp"
#include "../encryptDecrypt/Cryption.hpp"

ThreadPool::ThreadPool(size_t numThreads)
{
    if (numThreads == 0)
    {
        numThreads = 4;
    }
    for (size_t i = 0; i < numThreads; i++)
    {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queueLock);
        stopping = true;
    }
    taskAvailable.notify_all();
    for (auto &worker : workers)
    {
        worker.join();
    }
}

bool ThreadPool::submitToQueue(std::unique_ptr<Task> task)
{
    // workers reopen the file from the task string, so release this handle first
    std::string taskStr = task->toString();
    task.reset();
    {
        std::lock_guard<std::mutex> lock(queueLock);
        if (stopping)
        {
            return false;
        }
        tasks.push(taskStr);
    }
    taskAvailable.notify_one();
    return true;
}

void ThreadPool::waitForAll()
{
    std::unique_lock<std::mutex> lock(queueLock);
    allDone.wait(lock, [this]
                 { return tasks.empty() && activeTasks == 0; });
}

void ThreadPool::workerLoop()
{
    while (true)
    {
        std::string taskStr;
        {
            std::unique_lock<std::mutex> lock(queueLock);
            taskAvailable.wait(lock, [this]
                               { return stopping || !tasks.empty(); });
            if (stopping && tasks.empty())
            {
                return;
            }
            taskStr = tasks.front();
            tasks.pop();
            activeTasks++;
        }

        try
        {
            executeCryption(taskStr);
        }
        catch (const std::exception &ex)
        {
            std::cerr << "Task failed (" << taskStr << "): " << ex.what() << std::endl;
        }

        {
            std::lock_guard<std::mutex> lock(queueLock);
            activeTasks--;
            if (tasks.empty() && activeTasks == 0)
            {
                allDone.notify_all();
            }
        }
    }
}
