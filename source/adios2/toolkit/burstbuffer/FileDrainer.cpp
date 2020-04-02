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

#include <cerrno>
#include <cstring>
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

FileDrainOperation::FileDrainOperation(DrainOperation op,
                                       std::string &fromFileName,
                                       std::string &toFileName,
                                       size_t countBytes, size_t fromOffset,
                                       size_t toOffset, const void *data)
: op(op), fromFileName(fromFileName), toFileName(toFileName),
  countBytes(countBytes), fromOffset(fromOffset), toOffset(toOffset)
{
    if (data)
    {
        dataToWrite.resize(countBytes);
        std::memcpy(dataToWrite.data(), data, countBytes);
    };
}

/*FileDrainOperation::FileDrainOperation(std::string &toFileName,
                                       size_t countBytes, size_t toOffset, )
: op(DrainOperation::Write), fromFileName(""), toFileName(toFileName),
  countBytes(countBytes), append(false), fromOffset(0), toOffset(toOffset){

};*/

FileDrainer::FileDrainer() {}

FileDrainer::~FileDrainer() {}

void FileDrainer::AddOperation(FileDrainOperation &operation)
{
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

void FileDrainer::AddOperation(DrainOperation op, std::string &fromFileName,
                               std::string &toFileName, size_t fromOffset,
                               size_t toOffset, size_t countBytes,
                               const void *data)
{
    FileDrainOperation operation(op, fromFileName, toFileName, countBytes,
                                 fromOffset, toOffset, data);
    std::lock_guard<std::mutex> lockGuard(operationsMutex);
    operations.push(operation);
}

void FileDrainer::AddOperationSeekFrom(std::string &fromFileName,
                                       size_t fromOffset)
{
    std::string emptyStr;
    AddOperation(DrainOperation::SeekFrom, fromFileName, emptyStr, fromOffset,
                 0, 0);
}

void FileDrainer::AddOperationSeekTo(std::string &toFileName, size_t toOffset)
{
    std::string emptyStr;
    AddOperation(DrainOperation::SeekTo, emptyStr, toFileName, 0, toOffset, 0);
}

void FileDrainer::AddOperationSeekEnd(std::string &toFileName)
{
    std::string emptyStr;
    AddOperation(DrainOperation::SeekEnd, emptyStr, toFileName, 0, 0, 0);
}
void FileDrainer::AddOperationCopy(std::string &fromFileName,
                                   std::string &toFileName, size_t fromOffset,
                                   size_t toOffset, size_t countBytes)
{
    AddOperation(DrainOperation::Copy, fromFileName, toFileName, fromOffset,
                 toOffset, countBytes);
}
void FileDrainer::AddOperationCopyAppend(std::string &fromFileName,
                                         std::string &toFileName,
                                         size_t countBytes)
{
    AddOperation(DrainOperation::CopyAppend, fromFileName, toFileName, 0, 0,
                 countBytes);
}

void FileDrainer::AddOperationWriteAt(std::string &toFileName, size_t toOffset,
                                      size_t countBytes, const void *data)
{
    std::string emptyStr;
    AddOperation(DrainOperation::WriteAt, emptyStr, toFileName, 0, toOffset,
                 countBytes, data);
}

void FileDrainer::AddOperationWrite(std::string &toFileName, size_t countBytes,
                                    const void *data)
{
    std::string emptyStr;
    AddOperation(DrainOperation::Write, emptyStr, toFileName, 0, 0, countBytes,
                 data);
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
        fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        break;
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

void FileDrainer::Seek(int fd, size_t offset, const std::string &path,
                       int whence)
{
    const auto newPosition = lseek(fd, offset, whence);
    if (newPosition == -1)
    {
        throw std::ios_base::failure(
            "FileDrainer couldn't seek in file " + path +
            " to offset = " + std::to_string(offset) +
            " errno: " + std::to_string(errno) + std::strerror(errno) + "\n");
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
