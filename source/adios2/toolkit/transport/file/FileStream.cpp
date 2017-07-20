/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "FileStream.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> // std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transport
{

FileStream::FileStream(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "fstream", mpiComm, debugMode)
{
}

void FileStream::Open(const std::string &name, const OpenMode openMode)
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

    if (m_OpenMode == OpenMode::Write)
    {
        m_FileStream.open(name, std::fstream::out | std::fstream::binary);
    }
    else if (m_OpenMode == OpenMode::Append)
    {
        // to be changed to rw?
        m_FileStream.open(name, std::fstream::out | std::fstream::app |
                                    std::fstream::binary);
    }
    else if (m_OpenMode == OpenMode::Read)
    {
        m_FileStream.open(name, std::fstream::in | std::fstream::binary);
    }

    if (!m_FileStream)
    {
        throw std::ios_base::failure("ERROR: couldn't open file " + m_Name +
                                     ", in call to FileStream Open\n");
    }

    m_IsOpen = true;
}

void FileStream::SetBuffer(char *buffer, size_t size)
{
    m_FileStream.rdbuf()->pubsetbuf(buffer, size);

    if (!m_FileStream)
    {
        throw std::ios_base::failure("ERROR: couldn't set buffer in file " +
                                     m_Name +
                                     ", in call to FileStream SetBuffer\n");
    }
}

void FileStream::Write(const char *buffer, size_t size)
{
    auto lf_Write = [&](const char *buffer, size_t size) {

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("write").Resume();
        }
        m_FileStream.write(buffer, static_cast<std::streamsize>(size));

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("write").Pause();
        }

        if (!m_FileStream)
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name + ", in call to FILE* Write\n");
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

void FileStream::Flush()
{
    m_FileStream.flush();
    if (!m_FileStream)
    {
        throw std::ios_base::failure("ERROR: couldn't flush to file " + m_Name +
                                     ", in call to FileStream Flush\n");
    }
}

void FileStream::Close()
{
    m_FileStream.close();
    if (!m_FileStream)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to FileStream Close\n");
    }

    m_IsOpen = false;
}

} // end namespace transport
} // end namespace adios
