// Task.hpp
#ifndef TASK_HPP
#define TASK_HPP

#include "../fileHandling/IO.hpp"
#include <fstream>
#include <string>
#include <sstream>

enum class Action
{
    ENCRYPT,
    DECRYPT
};

struct Task
{
    std::string filePath;
    std::fstream f_stream;
    Action action;

    Task(std::fstream &&stream, Action act, std::string filePath) : filePath(filePath), f_stream(std::move(stream)), action(act) {}

    std::string toString() const
    {
        std::ostringstream oss;
        oss << filePath << "," << (action == Action::ENCRYPT ? "ENCRYPT" : "DECRYPT");
        return oss.str();
    }

    static Task fromString(const std::string &taskData)
    {
        // split on the last comma so file paths containing commas still work
        size_t sep = taskData.rfind(',');
        if (sep != std::string::npos)
        {
            std::string filePath = taskData.substr(0, sep);
            std::string actionStr = taskData.substr(sep + 1);
            Action action = (actionStr == "ENCRYPT") ? Action::ENCRYPT : Action::DECRYPT;
            IO io(filePath);
            std::fstream f_stream = io.getFileStream();
            if (f_stream.is_open())
            {
                return Task(std::move(f_stream), action, filePath);
            }
            else
            {
                throw std::runtime_error("Failed to open file: " + filePath);
            }
        }
        else
        {
            throw std::runtime_error("Invalid task data format");
        }
    }
};

#endif