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
/// \endcond

namespace adios
{
namespace transport
{

FileDescriptor::FileDescriptor(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "POSIX", mpiComm, debugMode)
{
}

FileDescriptor::~FileDescriptor()
{
    if (m_FileDescriptor != -1)
    {
        close(m_FileDescriptor);
    }
}

void FileDescriptor::Open(const std::string &name, const OpenMode openMode)
{
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
            throw std::ios_base::failure(
                "ERROR: couldn't open file " + m_Name +
                ", from call to Open in FileDescriptor transport using "
                "POSIX open. Does file exists?\n");
        }
    }

    m_IsOpen = true;
}

void FileDescriptor::Write(const char *buffer, std::size_t size)
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("write").Resume();
    }

    auto writtenSize = write(m_FileDescriptor, buffer, size);

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("write").Pause();
    }

    if (m_DebugMode)
    {
        if (writtenSize == -1)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't write to file " + m_Name +
                ", in call to POSIX FileDescriptor write\n");
        }

        if (static_cast<size_t>(writtenSize) != size)
        {
            throw std::ios_base::failure(
                "ERROR: written size + " + std::to_string(writtenSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to POSIX write\n");
        }
    }
}

void FileDescriptor::Flush() {}

void FileDescriptor::Close()
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("close").Resume();
    }

    int status = close(m_FileDescriptor);

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("close").Pause();
    }

    if (m_DebugMode)
    {
        if (status == -1)
        {
            throw std::ios_base::failure("ERROR: couldn't close file " +
                                         m_Name + ", in call to POSIX write\n");
        }
    }

    m_IsOpen = false;
}

} // end namespace transport
} // namespace adios
