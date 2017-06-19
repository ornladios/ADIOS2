/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FilePointer.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "FilePointer.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

namespace adios
{
namespace transport
{

FilePointer::FilePointer(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "stdio", mpiComm, debugMode)
{
}

FilePointer::~FilePointer()
{
    if (m_File)
    {
        fclose(m_File);
    }
}

void FilePointer::Open(const std::string &name, const OpenMode openMode)
{
    m_Name = name;
    m_OpenMode = openMode;

    if (m_OpenMode == OpenMode::Write)
    {
        m_File = fopen(name.c_str(), "w");
    }
    else if (m_OpenMode == OpenMode::Append)
    {
        m_File = fopen(name.c_str(), "a"); // need to change
    }
    else if (m_OpenMode == OpenMode::Read)
    {
        m_File = fopen(name.c_str(), "r");
    }

    if (m_DebugMode)
    {
        if (m_File == nullptr)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't open file " + name +
                ", "
                "in call to Open from stdio.h FilePointer* transport\n");
        }
    }
    m_IsOpen = true;
}

void FilePointer::SetBuffer(char *buffer, size_t size)
{
    int status = setvbuf(m_File, buffer, _IOFBF, size);

    if (m_DebugMode)
    {
        if (status == 1)
        {
            throw std::ios_base::failure(
                "ERROR: could not set buffer in rank " +
                std::to_string(m_RankMPI) + "\n");
        }
    }
}

void FilePointer::Write(const char *buffer, size_t size)
{
    fwrite(buffer, sizeof(char), size, m_File);

    if (m_DebugMode)
    {
        if (ferror(m_File))
        {
            throw std::ios_base::failure(
                "ERROR: couldn't write to file " + m_Name +
                ", in call to FilePointer* transport write\n");
        }
    }
}

void FilePointer::Flush() { fflush(m_File); }

void FilePointer::Close()
{
    fclose(m_File);

    m_IsOpen = false;
}

} // end namespace transport
} // namespace adios
