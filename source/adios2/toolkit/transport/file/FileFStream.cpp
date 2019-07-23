/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "FileFStream.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> // std::ios_base::failure
/// \endcond

namespace adios2
{
namespace transport
{

FileFStream::FileFStream(helper::Comm const &comm, const bool debugMode)
: Transport("File", "fstream", comm, debugMode)
{
}

void FileFStream::Open(const std::string &name, const Mode openMode)
{
    m_Name = name;
    CheckName();
    m_OpenMode = openMode;

    switch (m_OpenMode)
    {
    case (Mode::Write):
        ProfilerStart("open");
        m_FileStream.open(name, std::fstream::out | std::fstream::binary |
                                    std::fstream::trunc);
        ProfilerStop("open");
        break;

    case (Mode::Append):
        ProfilerStart("open");
        // m_FileStream.open(name, std::fstream::in | std::fstream::out |
        //                            std::fstream::binary);
        m_FileStream.open(name, std::fstream::in | std::fstream::out |
                                    std::fstream::binary);
        m_FileStream.seekp(0, std::ios_base::end);
        ProfilerStop("open");
        break;

    case (Mode::Read):
        ProfilerStart("open");
        m_FileStream.open(name, std::fstream::in | std::fstream::binary);
        ProfilerStop("open");
        break;

    default:
        CheckFile("unknown open mode for file " + m_Name +
                  ", in call to stream open");
    }

    CheckFile("couldn't open file " + m_Name +
              ", check permissions or path existence, in call to fstream open");
    m_IsOpen = true;
}

void FileFStream::SetBuffer(char *buffer, size_t size)
{
    m_FileStream.rdbuf()->pubsetbuf(buffer, size);
    CheckFile("couldn't set buffer in file " + m_Name +
              ", in call to fstream rdbuf()->pubsetbuf");
}

void FileFStream::Write(const char *buffer, size_t size, size_t start)
{
    auto lf_Write = [&](const char *buffer, size_t size) {
        ProfilerStart("write");
        m_FileStream.write(buffer, static_cast<std::streamsize>(size));
        ProfilerStop("write");
        CheckFile("couldn't write from file " + m_Name +
                  ", in call to fstream write");
    };

    if (start != MaxSizeT)
    {
        m_FileStream.seekp(start);
        CheckFile("couldn't move to start position " + std::to_string(start) +
                  " in file " + m_Name + ", in call to fstream seekp");
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

void FileFStream::Read(char *buffer, size_t size, size_t start)
{
    auto lf_Read = [&](char *buffer, size_t size) {
        ProfilerStart("read");
        m_FileStream.read(buffer, static_cast<std::streamsize>(size));
        ProfilerStop("read");
        CheckFile("couldn't read from file " + m_Name +
                  ", in call to fstream read");
    };

    if (start != MaxSizeT)
    {
        m_FileStream.seekg(start);
        CheckFile("couldn't move to start position " + std::to_string(start) +
                  " in file " + m_Name + ", in call to fstream seekg");
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

size_t FileFStream::GetSize()
{
    const auto currentPosition = m_FileStream.tellg();
    m_FileStream.seekg(0, std::ios_base::end);
    const std::streampos size = m_FileStream.tellg();
    if (static_cast<int>(size) == -1)
    {
        throw std::ios_base::failure("ERROR: couldn't get size of " + m_Name +
                                     " file\n");
    }
    m_FileStream.seekg(currentPosition);
    return static_cast<size_t>(size);
}

void FileFStream::Flush()
{
    ProfilerStart("write");
    m_FileStream.flush();
    ProfilerStart("write");
    CheckFile("couldn't flush to file " + m_Name +
              ", in call to fstream flush");
}

void FileFStream::Close()
{
    ProfilerStart("close");
    m_FileStream.close();
    ProfilerStop("close");

    CheckFile("couldn't close file " + m_Name + ", in call to fstream close");
    m_IsOpen = false;
}

void FileFStream::CheckFile(const std::string hint) const
{
    if (!m_FileStream)
    {
        throw std::ios_base::failure("ERROR: " + hint + "\n");
    }
}

void FileFStream::SeekToEnd()
{
    m_FileStream.seekp(0, std::ios_base::end);
    CheckFile("couldn't move to the end of file " + m_Name +
              ", in call to fstream seekp");
}

void FileFStream::SeekToBegin()
{
    m_FileStream.seekp(0, std::ios_base::beg);
    CheckFile("couldn't move to the beginning of file " + m_Name +
              ", in call to fstream seekp");
}

} // end namespace transport
} // end namespace adios2
