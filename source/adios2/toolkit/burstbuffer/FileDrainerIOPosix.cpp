/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainerIOPosix.cpp
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include "FileDrainerIOPosix.h"

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <chrono>
#include <cstdio>      // remove
#include <cstring>     // std::memcpy, strerror
#include <errno.h>     // errno
#include <fcntl.h>     // open
#include <stddef.h>    // write output
#include <sys/stat.h>  // open, fstat
#include <sys/types.h> // open
#include <thread>      // std::this_thread::sleep_for
#include <unistd.h>    // write, close

namespace adios2
{
namespace burstbuffer
{

InputFile FileDrainerIO::GetFileForRead(const std::string &path)
{
    auto it = m_InputFileMap.find(path);
    if (it != m_InputFileMap.end())
    {
        return it->second;
    }
    else
    {
        InputFile f = std::make_shared<PosixInputFile>();
        m_InputFileMap.emplace(path, f);
        Open(f, path);
        return f;
    }
}

OutputFile FileDrainerIO::GetFileForWrite(const std::string &path, bool append)
{
    auto it = m_OutputFileMap.find(path);
    if (it != m_OutputFileMap.end())
    {
        return it->second;
    }
    else
    {
        OutputFile f = std::make_shared<PosixOutputFile>();
        m_OutputFileMap.emplace(path, f);
        Open(f, path, append);
        return f;
    }
}

std::string FileDrainerIO::SysErrMsg(const int errorNumber) const
{
    return std::string(": errno = " + std::to_string(errorNumber) + ": " +
                       strerror(errorNumber));
}

void FileDrainerIO::Open(InputFile &f, const std::string &path)
{
    errno = 0;
    f->m_fd = open(path.c_str(), O_RDONLY);
    f->m_Errno = errno;
    f->m_Path = path;
}

void FileDrainerIO::Open(OutputFile &f, const std::string &path, bool append)
{
    errno = 0;
    if (append)
    {
        f->m_fd = open(path.c_str(), O_RDWR | O_CREAT, 0777);
        lseek64(f->m_fd, 0, SEEK_END);
    }
    else
    {
        f->m_fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }
    f->m_Errno = errno;
    f->m_Path = path;
}

void FileDrainerIO::Close(InputFile &f)
{
    errno = 0;
    close(f->m_fd);
    f->m_Errno = errno;
}
void FileDrainerIO::Close(OutputFile &f)
{
    errno = 0;
    close(f->m_fd);
    f->m_Errno = errno;
}

bool FileDrainerIO::Good(InputFile &f) { return (f->m_Errno == 0); }
bool FileDrainerIO::Good(OutputFile &f) { return (f->m_Errno == 0); }

void FileDrainerIO::CloseAll()
{
    for (auto it = m_OutputFileMap.begin(); it != m_OutputFileMap.end(); ++it)
    {
        // if (it->second->good())
        //{
        Close(it->second);
        //}
    }
    m_OutputFileMap.clear();
    for (auto it = m_InputFileMap.begin(); it != m_InputFileMap.end(); ++it)
    {
        // if (it->second->good())
        //{
        Close(it->second);
        //}
    }
    m_InputFileMap.clear();
}

void FileDrainerIO::Seek(InputFile &f, size_t offset, const std::string &path)
{
    lseek64(f->m_fd, static_cast<off64_t>(offset), SEEK_SET);
}

void FileDrainerIO::Seek(OutputFile &f, size_t offset, const std::string &path)
{
    lseek64(f->m_fd, static_cast<off64_t>(offset), SEEK_SET);
}

void FileDrainerIO::SeekEnd(OutputFile &f) { lseek64(f->m_fd, 0, SEEK_END); }

size_t FileDrainerIO::GetFileSize(InputFile &f)
{
    struct stat fileStat;
    errno = 0;
    if (fstat(f->m_fd, &fileStat) == -1)
    {
        f->m_Errno = errno;
        throw std::ios_base::failure("ERROR: couldn't get size of file " +
                                     f->m_Path + SysErrMsg(errno));
    }
    f->m_Errno = errno;
    return static_cast<size_t>(fileStat.st_size);
}

std::pair<size_t, double> FileDrainerIO::Read(InputFile &f, size_t count,
                                              char *buffer,
                                              const std::string &path)
{
    size_t totalRead = 0;
    double totalSlept = 0.0;
    const double sleepUnit = 0.01; // seconds
    while (count > 0)
    {
        const auto currentOffset = lseek64(f->m_fd, 0, SEEK_CUR);
        size_t readCount =
            (count < DefaultMaxFileBatchSize ? count : DefaultMaxFileBatchSize);
        errno = 0;
        const auto readSize = read(f->m_fd, buffer, readCount);
        if (readSize == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                throw std::ios_base::failure(
                    "FileDrainer couldn't read from file " + path +
                    " offset = " + std::to_string(currentOffset) +
                    " count = " + std::to_string(count) + " bytes but only " +
                    std::to_string(totalRead + readSize) + ".\n");
            }
        }
        else if (readSize < static_cast<ssize_t>(count))
        {
            // need to wait for more data to come
            std::chrono::duration<double> d(sleepUnit);
            std::this_thread::sleep_for(d);
            totalSlept += sleepUnit;
        }
        buffer += readSize;
        count -= readSize;
        totalRead += readSize;
    }
    return std::pair<size_t, double>(totalRead, totalSlept);
}

size_t FileDrainerIO::Write(OutputFile &f, size_t count, const char *buffer,
                            const std::string &path)
{
    size_t totalWritten = 0;
    while (count > 0)
    {
        const auto currentOffset = lseek64(f->m_fd, 0, SEEK_CUR);
        size_t writeCount =
            (count < DefaultMaxFileBatchSize ? count : DefaultMaxFileBatchSize);
        errno = 0;
        const auto writtenSize = write(f->m_fd, buffer, writeCount);
        if (writtenSize == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                throw std::ios_base::failure(
                    "FileDrainer couldn't write to file " + path +
                    " offset = " + std::to_string(currentOffset) +
                    " count = " + std::to_string(count) + " bytes but only " +
                    std::to_string(totalWritten + writtenSize) + ".\n");
            }
        }
        buffer += writtenSize;
        count -= writtenSize;
        totalWritten += writtenSize;
    }

    return totalWritten;
}

int FileDrainerIO::FileSync(OutputFile &f)
{
#if (_POSIX_C_SOURCE >= 199309L || _XOPEN_SOURCE >= 500)
    return fdatasync(f->m_fd);
#else
    return fsync(f->m_fd);
#endif
}

void FileDrainerIO::Delete(OutputFile &f, const std::string &path)
{
    Close(f);
    std::remove(path.c_str());
}

void FileDrainerIO::SetVerbose(int verboseLevel, int rank)
{
    m_Verbose = verboseLevel;
    m_Rank = rank;
}

} // end namespace burstbuffer
} // end namespace adios2
