/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.cpp file I/O using POSIX I/O library
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "FilePOSIX.h"

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
namespace transport
{

FilePOSIX::FilePOSIX(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "POSIX", mpiComm, debugMode)
{
}

FilePOSIX::~FilePOSIX()
{
    if (m_IsOpen)
    {
        close(m_FileDescriptor);
    }
}

void FilePOSIX::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    CheckName();
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {

    case (Mode::Write):
        ProfilerStart("open");
        MkDir(m_Name);
        m_FileDescriptor = open(m_Name.c_str(), O_WRONLY | O_CREAT, 0777);
        ProfilerStop("open");
        break;

    case (Mode::Append):
        ProfilerStart("open");
        m_FileDescriptor = open(m_Name.c_str(), O_RDWR);
        ProfilerStop("open");
        break;

    case (Mode::Read):
        ProfilerStart("open");
        m_FileDescriptor = open(m_Name.c_str(), O_RDONLY);
        ProfilerStop("open");
        break;

    default:
        CheckFile("unknown open mode for file " + m_Name +
                  ", in call to POSIX open");
    }

    CheckFile("couldn't open file " + m_Name +
              ", check permissions or path existence, in call to POSIX open");

    m_IsOpen = true;
}

void FilePOSIX::Write(const char *buffer, size_t size, size_t start)
{
    auto lf_Write = [&](const char *buffer, size_t size) {

        ProfilerStart("write");
        const auto writtenSize = write(m_FileDescriptor, buffer, size);
        ProfilerStop("write");

        if (writtenSize == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name +
                                         ", in call to FileDescriptor Write\n");
        }

        if (static_cast<size_t>(writtenSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: written size + " + std::to_string(writtenSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to FileDescriptor Write\n");
        }
    };

    if (start != MaxSizeT)
    {
        const auto newPosition = lseek(m_FileDescriptor, start, SEEK_SET);

        if (static_cast<size_t>(newPosition) != start)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't move to start position " +
                std::to_string(start) + " in file " + m_Name +
                ", in call to POSIX lseek\n");
        }
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Write(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Write(&buffer[position], remainder);
    }
    else
    {
        lf_Write(buffer, size);
    }
}

void FilePOSIX::Read(char *buffer, size_t size, size_t start)
{
    auto lf_Read = [&](char *buffer, size_t size) {

        ProfilerStart("read");
        const auto readSize = read(m_FileDescriptor, buffer, size);
        ProfilerStop("read");

        if (readSize == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't read from file " +
                                         m_Name +
                                         ", in call to POSIX IO read\n");
        }

        if (static_cast<size_t>(readSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: read size + " + std::to_string(readSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to POSIX IO read\n");
        }
    };

    if (start != MaxSizeT)
    {
        const auto newPosition = lseek(m_FileDescriptor, start, SEEK_SET);

        if (static_cast<size_t>(newPosition) != start)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't move to start position " +
                std::to_string(start) + " in file " + m_Name +
                ", in call to POSIX lseek\n");
        }
    }

    if (size > DefaultMaxFileBatchSize)
    {
        const size_t batches = size / DefaultMaxFileBatchSize;
        const size_t remainder = size % DefaultMaxFileBatchSize;

        size_t position = 0;
        for (size_t b = 0; b < batches; ++b)
        {
            lf_Read(&buffer[position], DefaultMaxFileBatchSize);
            position += DefaultMaxFileBatchSize;
        }
        lf_Read(&buffer[position], remainder);
    }
    else
    {
        lf_Read(buffer, size);
    }
}

size_t FilePOSIX::GetSize()
{
    struct stat fileStat;
    if (fstat(m_FileDescriptor, &fileStat) == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't get size of file " +
                                     m_Name + "\n");
    }
    return static_cast<size_t>(fileStat.st_size);
}

void FilePOSIX::Flush() {}

void FilePOSIX::Close()
{
    ProfilerStart("close");
    const int status = close(m_FileDescriptor);
    ProfilerStop("close");

    if (status == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to POSIX IO close\n");
    }

    m_IsOpen = false;
}

void FilePOSIX::CheckFile(const std::string hint) const
{
    if (m_FileDescriptor == -1)
    {
        throw std::ios_base::failure("ERROR: " + hint + "\n");
    }
}

} // end namespace transport
} // end namespace adios2
