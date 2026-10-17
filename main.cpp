#include <iostream>
#include <filesystem>
#include "./src/app/processes/ProcessManagement.hpp"
#include "./src/app/processes/ThreadPool.hpp"
#include "./src/app/processes/Task.hpp"
#include <ctime>
#include <iomanip>

namespace fs = std::filesystem;

// Works with either ProcessManagement or ThreadPool, both expose submitToQueue
template <typename Executor>
void submitDirectory(Executor &executor, const std::string &directory, Action taskAction)
{
    for (const auto &entry : fs::recursive_directory_iterator(directory))
    {
        // never touch the key file, or nothing could be decrypted afterwards
        if (entry.is_regular_file() && entry.path().filename() != ".env")
        {
            std::string filePath = entry.path().string();
            IO io(filePath);
            std::fstream f_stream = io.getFileStream();

            if (f_stream.is_open())
            {
                auto task = std::make_unique<Task>(std::move(f_stream), taskAction, filePath);

                std::time_t t = std::time(nullptr);
                std::tm now;
                localtime_r(&t, &now);
                std::cout << "Starting the encryption/decryption at: " << std::put_time(&now, "%Y-%m-%d %H:%M:%S") << std::endl;
                executor.submitToQueue(std::move(task));
            }
            else
            {
                std::cout << "Unable to open file: " << filePath << std::endl;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    std::string directory;
    std::string action;
    std::string mode;

    std::cout << "Enter the directory path: ";
    std::getline(std::cin, directory);

    std::cout << "Enter the action (encrypt/decrypt): ";
    std::getline(std::cin, action);

    std::cout << "Enter the mode (process/thread): ";
    std::getline(std::cin, mode);

    if (action != "encrypt" && action != "decrypt")
    {
        std::cout << "Invalid action! Use exactly 'encrypt' or 'decrypt'." << std::endl;
        return 1;
    }
    if (mode != "process" && mode != "thread")
    {
        std::cout << "Invalid mode! Use exactly 'process' or 'thread'." << std::endl;
        return 1;
    }

    try
    {
        if (fs::exists(directory) && fs::is_directory(directory))
        {
            Action taskAction = (action == "encrypt") ? Action::ENCRYPT : Action::DECRYPT;

            if (mode == "thread")
            {
                ThreadPool threadPool;
                submitDirectory(threadPool, directory, taskAction);
                threadPool.waitForAll();
            }
            else
            {
                ProcessManagement processManagement;
                submitDirectory(processManagement, directory, taskAction);
                processManagement.waitForAll();
            }
        }
        else
        {
            std::cout << "Invalid directory path!" << std::endl;
        }
    }
    catch (const fs::filesystem_error &ex)
    {
        std::cout << "Filesystem error: " << ex.what() << std::endl;
    }
    catch (const std::runtime_error &ex)
    {
        std::cout << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
