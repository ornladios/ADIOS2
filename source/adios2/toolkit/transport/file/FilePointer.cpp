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

// removes fopen warning on Windows
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_DEPRECATE
#endif

namespace adios2
{
namespace transport
{

FilePointer::FilePointer(MPI_Comm mpiComm, const bool debugMode)
: Transport("File", "stdio", mpiComm, debugMode)
{
}

FilePointer::~FilePointer()
{
    if (m_IsOpen)
    {
        fclose(m_File);
    }
}

void FilePointer::Open(const std::string &name, const OpenMode openMode)
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
        m_File = fopen(name.c_str(), "wb");
    }
    else if (m_OpenMode == OpenMode::Append)
    {
        // need to change when implemented
        m_File = fopen(name.c_str(), "ab");
    }
    else if (m_OpenMode == OpenMode::Read)
    {
        m_File = fopen(name.c_str(), "rb");
    }

    if (ferror(m_File))
    {
        throw std::ios_base::failure("ERROR: couldn't open file " + name +
                                     ", "
                                     "in call to FilePointer Open\n");
    }

    m_IsOpen = true;
}

void FilePointer::SetBuffer(char *buffer, size_t size)
{
    const int status = setvbuf(m_File, buffer, _IOFBF, size);

    if (!status)
    {
        throw std::ios_base::failure(
            "ERROR: could not set FILE* buffer in file " + m_Name +
            ", in call to FilePointer SetBuffer\n");
    }
}

void FilePointer::Write(const char *buffer, size_t size)
{
    auto lf_Write = [&](const char *buffer, size_t size) {

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("write").Resume();
        }
        auto writtenSize = fwrite(buffer, sizeof(char), size, m_File);

        if (m_Profiler.IsActive)
        {
            m_Profiler.Timers.at("write").Pause();
        }

        if (std::ferror(m_File))
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name + ", in call to FILE* Write\n");
        }

        if (writtenSize != size)
        {
            throw std::ios_base::failure(
                "ERROR: written size + " + std::to_string(writtenSize) +
                " is not equal to intended size " + std::to_string(size) +
                " in file " + m_Name + ", in call to FilePointer Write\n");
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

void FilePointer::Flush()
{
    const int status = fflush(m_File);

    if (status == EOF)
    {
        throw std::ios_base::failure("ERROR: couldn't flush file " + m_Name +
                                     ", in call to FilePointer Flush\n");
    }
}

void FilePointer::Close()
{
    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("close").Resume();
    }

    const int status = fclose(m_File);

    if (m_Profiler.IsActive)
    {
        m_Profiler.Timers.at("close").Pause();
    }

    if (status == EOF)
    {
        throw std::ios_base::failure("ERROR: couldn't close file " + m_Name +
                                     ", in call to FilePointer Write\n");
    }

    m_IsOpen = false;
}

} // end namespace transport
} // end namespace adios
