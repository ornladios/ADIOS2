/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDescriptor.cpp file I/O using POSIX I/O library
 *
 *  Created on: Oct 6, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "FileDescriptor.h"

#include <fcntl.h>     // open
#include <stddef.h>    // write output
#include <sys/stat.h>  // open
#include <sys/types.h> // open
#include <unistd.h>    // write, close

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
#include <iostream>
/// \endcond

namespace adios2
{
namespace transport
{

FileDescriptor::FileDescriptor(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "POSIX", mpiComm, debugMode)
{
}

FileDescriptor::~FileDescriptor()
{
    if (m_IsOpen)
    {
        close(m_FileDescriptor);
    }
}

void FileDescriptor::Open(const std::string &name, const OpenMode openMode)
{
    if (m_DebugMode)
    {
        if (name.empty())
        {
            throw std::invalid_argument(
                "ERROR: file name is empty, in call to FilePointer Open\n");
        }
    }

    m_Name = name;
    m_OpenMode = openMode;

    if (openMode == OpenMode::Write)
    {
        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("open").Resume();
        }

        m_FileDescriptor = open(m_Name.c_str(), O_WRONLY | O_CREAT, 0777);

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("open").Pause();
        }
    }
    else if (openMode == OpenMode::Append)
    {
        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("open").Resume();
        }

        // TODO we need to change this to read/write
        m_FileDescriptor = open(m_Name.c_str(), O_WRONLY | O_APPEND);

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("open").Pause();
        }
    }
    else if (openMode == OpenMode::Read)
    {
        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("open").Resume();
        }

        m_FileDescriptor = open(m_Name.c_str(), O_RDONLY);

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("open").Pause();
        }
    }

    if (m_DebugMode)
    {
        if (m_FileDescriptor == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't open file " + m_Name +
                                         ", check permissions or existence, in "
                                         "call to FileDescriptor Open\n");
        }
    }

    m_IsOpen = true;
}

void FileDescriptor::Write(const char *buffer, size_t size)
{
    auto lf_Write = [&](const char *buffer, size_t size) {

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("write").Resume();
        }
        auto writtenSize = write(m_FileDescriptor, buffer, size);
        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("write").Pause();
        }

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

void FileDescriptor::Flush() {}

void FileDescriptor::Close()
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("close").Resume();
    }

    const int status = close(m_FileDescriptor);

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("close").Pause();
    }

    if (status == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to FileDescriptor Close\n");
    }

    m_IsOpen = false;
}

} // end namespace transport
} // end namespace adios
