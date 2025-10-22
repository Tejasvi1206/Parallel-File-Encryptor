#ifndef IO_HPP
#define IO_HPP

#include <fstream>
#include <string>
#include <iostream>

class IO
{
public:
    IO(const std::string &file_path);
    ~IO();
    std::fstream getFileStream();

private:
    std::fstream file_stream; // stores file after we read it getter-setter type
};

#endif