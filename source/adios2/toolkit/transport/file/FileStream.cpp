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

namespace adios
{
namespace transport
{

FileStream::FileStream(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "fstream", mpiComm, debugMode)
{
}

void FileStream::Open(const std::string &name, const OpenMode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;

    if (m_OpenMode == OpenMode::Write)
    {
        m_FileStream.open(name, std::fstream::out);
    }
    else if (m_OpenMode == OpenMode::Append)
    {
        // to be changed to rw?
        m_FileStream.open(name, std::fstream::out | std::fstream::app);
    }
    else if (m_OpenMode == OpenMode::Read)
    {
        m_FileStream.open(name, std::fstream::in);
    }

    if (m_DebugMode)
    {
        if (!m_FileStream)
        {
            throw std::ios_base::failure("ERROR: couldn't open file " + name +
                                         ", in call to FileStream Open\n");
        }
    }
    m_IsOpen = true;
}

void FileStream::SetBuffer(char *buffer, size_t size)
{
    m_FileStream.rdbuf()->pubsetbuf(buffer, size);
    if (m_DebugMode)
    {
        if (!m_FileStream)
        {
            throw std::ios_base::failure("ERROR: couldn't SetBuffer to file " +
                                         m_Name +
                                         ", in call to FileStream SetBuffer\n");
        }
    }
}

void FileStream::Write(const char *buffer, size_t size)
{
    m_FileStream.write(buffer, size);

    if (m_DebugMode)
    {
        if (!m_FileStream)
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name +
                                         ", in call to FileStream write\n");
        }
    }
}

void FileStream::Flush()
{
    m_FileStream.flush();
    if (m_DebugMode)
    {
        if (!m_FileStream)
        {
            throw std::ios_base::failure("ERROR: couldn't flush to file " +
                                         m_Name +
                                         ", in call to FileStream Flush\n");
        }
    }
}

void FileStream::Close()
{
    m_FileStream.close();
    if (m_DebugMode)
    {
        if (!m_FileStream)
        {
            throw std::ios_base::failure("ERROR: couldn't close file " +
                                         m_Name +
                                         ", in call to FileStream Close\n");
        }
    }
    m_IsOpen = false;
}

} // end namespace transport
} // end namespace adios
