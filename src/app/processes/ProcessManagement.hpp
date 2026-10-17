#ifndef PROCESS_MANAGEMENT_HPP
#define PROCESS_MANAGEMENT_HPP

#include "Task.hpp"
#include <iostream>
#include <memory>
#include <atomic>
#include <semaphore.h>

class ProcessManagement
{
    sem_t *itemsSemaphore;
    sem_t *emptySlotsSemaphore;
    // binary semaphore used as a lock; a std::mutex is not shared across fork()
    sem_t *queueMutexSemaphore;

public:
    ProcessManagement();
    ~ProcessManagement();
    bool submitToQueue(std::unique_ptr<Task> task);
    void executeTask();
    void waitForAll();

private:
    static const int QUEUE_CAPACITY = 1000;
    static const int TASK_SIZE = 256;

    struct SharedMemory
    {
        std::atomic<int> size;
        char tasks[QUEUE_CAPACITY][TASK_SIZE];
        int front;
        int rear;

        void printSharedMemory()
        {
            std::cout << size << std::endl;
            std::cout << front << std::endl;
            std::cout << rear << std::endl;
        }
    };
    SharedMemory *sharedMem;
    int shmFd;
    const char *SHM_NAME = "/my_queue";
};

#endif
