#include <iostream>
#include "ProcessManagement.hpp"
#include <unistd.h>
#include <cstring>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include "../encryptDecrypt/Cryption.hpp"
#include <sys/mman.h>
#include <atomic>
#include <semaphore.h>
#include <stdexcept>

ProcessManagement::ProcessManagement()
{
    // remove leftovers from a previous run so the initial counts below apply
    sem_unlink("/items_semaphore");
    sem_unlink("/empty_slots_semaphore");
    sem_unlink("/queue_mutex_semaphore");
    itemsSemaphore = sem_open("/items_semaphore", O_CREAT, 0666, 0);
    emptySlotsSemaphore = sem_open("/empty_slots_semaphore", O_CREAT, 0666, QUEUE_CAPACITY);
    queueMutexSemaphore = sem_open("/queue_mutex_semaphore", O_CREAT, 0666, 1);
    if (itemsSemaphore == SEM_FAILED || emptySlotsSemaphore == SEM_FAILED || queueMutexSemaphore == SEM_FAILED)
    {
        throw std::runtime_error("Failed to create semaphores");
    }

    shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shmFd < 0 || ftruncate(shmFd, sizeof(SharedMemory)) < 0)
    {
        throw std::runtime_error("Failed to create shared memory");
    }
    void *mem = mmap(nullptr, sizeof(SharedMemory), PROT_READ | PROT_WRITE, MAP_SHARED, shmFd, 0);
    if (mem == MAP_FAILED)
    {
        throw std::runtime_error("Failed to map shared memory");
    }
    sharedMem = static_cast<SharedMemory *>(mem);
    sharedMem->front = 0;
    sharedMem->rear = 0;
    sharedMem->size.store(0);
}

ProcessManagement::~ProcessManagement()
{
    // children still use the queue, so they must finish before it is torn down
    waitForAll();
    munmap(sharedMem, sizeof(SharedMemory));
    close(shmFd);
    shm_unlink(SHM_NAME);
    sem_close(itemsSemaphore);
    sem_close(emptySlotsSemaphore);
    sem_close(queueMutexSemaphore);
    sem_unlink("/items_semaphore");
    sem_unlink("/empty_slots_semaphore");
    sem_unlink("/queue_mutex_semaphore");
}

bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task)
{
    std::string taskStr = task->toString();
    if (taskStr.size() >= TASK_SIZE)
    {
        std::cout << "File path too long, skipping: " << task->filePath << std::endl;
        return false;
    }

    sem_wait(emptySlotsSemaphore);
    sem_wait(queueMutexSemaphore);
    strcpy(sharedMem->tasks[sharedMem->rear], taskStr.c_str());
    sharedMem->rear = (sharedMem->rear + 1) % QUEUE_CAPACITY;
    sharedMem->size.fetch_add(1);
    sem_post(queueMutexSemaphore);
    sem_post(itemsSemaphore);

    int pid = fork();
    if (pid < 0)
    {
        // no child to consume the queued task, so handle it here
        executeTask();
    }
    else if (pid == 0)
    {
        executeTask();
        exit(0);
    }
    return true;
}

void ProcessManagement::executeTask()
{
    sem_wait(itemsSemaphore);
    sem_wait(queueMutexSemaphore);
    char taskStr[TASK_SIZE];
    strcpy(taskStr, sharedMem->tasks[sharedMem->front]);
    sharedMem->front = (sharedMem->front + 1) % QUEUE_CAPACITY;
    sharedMem->size.fetch_sub(1);
    sem_post(queueMutexSemaphore);
    sem_post(emptySlotsSemaphore);

    try
    {
        executeCryption(taskStr);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Task failed (" << taskStr << "): " << ex.what() << std::endl;
    }
}

void ProcessManagement::waitForAll()
{
    while (wait(nullptr) > 0)
    {
    }
}
