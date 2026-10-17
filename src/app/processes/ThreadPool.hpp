#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include "Task.hpp"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();
    bool submitToQueue(std::unique_ptr<Task> task);
    void waitForAll();

private:
    void workerLoop();

    std::vector<std::thread> workers;
    std::queue<std::string> tasks;
    std::mutex queueLock;
    std::condition_variable taskAvailable;
    std::condition_variable allDone;
    size_t activeTasks = 0;
    bool stopping = false;
};

#endif
