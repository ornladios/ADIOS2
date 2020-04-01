/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainer.cpp
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include "FileDrainer.h"

#include <fcntl.h>     // open
#include <stddef.h>    // write output
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <unistd.h>    // write, close

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios2
{
namespace burstbuffer
{

FileDrainer::FileDrainer() {}

FileDrainer::~FileDrainer() {}

void FileDrainer::AddOperation(FileDrainOperation &operation)
{
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

int FileDrainer::GetFileDescriptor(const std::string &path, const Mode mode)
{
    int fd;
    auto it = fileDescriptorMap.find(path);
    if (it != fileDescriptorMap.end())
    {
        return it->second;
    }
    else
    {
        fd = Open(path, mode);
        fileDescriptorMap.emplace(path, fd);
    }
    return fd;
}

int FileDrainer::Open(const std::string &path, const Mode mode)
{
    int fd;
    switch (mode)
    {
    case (Mode::Write):
    case (Mode::Append):

        fd = open(path.c_str(), O_RDWR | O_CREAT, 0777);
        lseek(fd, 0, SEEK_END);
        break;

    case (Mode::Read):
    default:
        fd = open(path.c_str(), O_RDONLY);
        break;
    }
    return fd;
}

void FileDrainer::Close(int fd, const std::string &path)
{
    const int status = close(fd);

    if (status == -1)
    {
        throw std::ios_base::failure("FileDrainer couldn't close file " + path +
                                     "\n");
    }
}

void FileDrainer::CloseAll()
{
    std::map<std::string, int>::iterator it;
    for (it = fileDescriptorMap.begin(); it != fileDescriptorMap.end(); ++it)
    {
        if (it->second != errorState)
        {
            Close(it->second, it->first);
        }
        fileDescriptorMap.erase(it);
    }
}

void FileDrainer::Seek(int fd, size_t offset, const std::string &path)
{
    const auto newPosition = lseek(fd, offset, SEEK_SET);
    if (newPosition == -1)
    {
        throw std::ios_base::failure(
            "FileDrainer couldn't seek in file " + path +
            " to offset = " + std::to_string(offset) + "\n");
    }
}

void FileDrainer::Read(int fd, size_t count, char *buffer,
                       const std::string &path)
{
    while (count > 0)
    {
        const auto readSize = read(fd, buffer, count);

        if (readSize == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw std::ios_base::failure(
                "FileDrainer couldn't read from file " + path +
                " count = " + std::to_string(count) + " bytes\n");
            break;
        }

        buffer += readSize;
        count -= readSize;
    }
}

void FileDrainer::Write(int fd, size_t count, const char *buffer,
                        const std::string &path)
{
    while (count > 0)
    {
        const auto writtenSize = write(fd, buffer, count);

        if (writtenSize == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            throw std::ios_base::failure(
                "FileDrainer couldn't write to file " + path +
                " count = " + std::to_string(count) + " bytes\n");
            break;
        }

        buffer += writtenSize;
        count -= writtenSize;
    }
}

} // end namespace burstbuffer
} // end namespace adios2
