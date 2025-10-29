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

ProcessManagement::ProcessManagement()
{
    // Initialize semaphores as class members
    itemsSemaphore = sem_open("/items_semaphore", O_CREAT, 0666, 0);
    emptySlotsSemaphore = sem_open("/empty_slots_semaphore", O_CREAT, 0666, 1000);

    if (itemsSemaphore == SEM_FAILED || emptySlotsSemaphore == SEM_FAILED)
    {
        std::cerr << "Semaphore initialization failed!" << std::endl;
        exit(1);
    }

    // Create shared memory
    shmFd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shmFd < 0)
    {
        std::cerr << "Shared memory creation failed!" << std::endl;
        exit(1);
    }

    ftruncate(shmFd, sizeof(SharedMemory));
    sharedMem = static_cast<SharedMemory *>(mmap(
        nullptr,
        sizeof(SharedMemory),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shmFd,
        0));

    if (sharedMem == MAP_FAILED)
    {
        std::cerr << "Mapping shared memory failed!" << std::endl;
        exit(1);
    }

    sharedMem->front = 0;
    sharedMem->rear = 0;
    sharedMem->size.store(0);
}

ProcessManagement::~ProcessManagement()
{
    sem_close(itemsSemaphore);
    sem_close(emptySlotsSemaphore);
    sem_unlink("/items_semaphore");
    sem_unlink("/empty_slots_semaphore");

    munmap(sharedMem, sizeof(SharedMemory));
    shm_unlink(SHM_NAME);
}

bool ProcessManagement::submitToQueue(std::unique_ptr<Task> task)
{
    sem_wait(emptySlotsSemaphore);
    strcpy(sharedMem->tasks[sharedMem->rear], task->toString().c_str());
    sharedMem->rear = (sharedMem->rear + 1) % 1000;
    sharedMem->size.fetch_add(1);

    sem_post(itemsSemaphore);
    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "Fork failed!" << std::endl;
        return false;
    }
    else if (pid == 0)
    {
        // Child
        executeTask();
        exit(0);
    }
    else
    {
        waitpid(pid, nullptr, 0);
    }

    return true;
}

void ProcessManagement::executeTask()
{
    sem_wait(itemsSemaphore);

    char taskStr[256];
    strcpy(taskStr, sharedMem->tasks[sharedMem->front]);
    sharedMem->front = (sharedMem->front + 1) % 1000;
    sharedMem->size.fetch_sub(1);

    sem_post(emptySlotsSemaphore); // free a slot

    executeCryption(taskStr);
}
