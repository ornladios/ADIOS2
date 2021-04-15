/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * FileDrainer.cpp
 *
 *  Created on: April 1, 2020
 *      Author: Norbert Podhorszki <pnorbert@ornl.gov>
 */

#include "FileDrainerIOFstream.h"

#include <chrono>
#include <cstdio>
#include <cstring> // std::memcpy
#include <thread>  // std::this_thread::sleep_for

/// \cond EXCLUDE_FROM_DOXYGEN
#include <ios> //std::ios_base::failure
/// \endcond

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
        InputFile f = std::make_shared<std::ifstream>();
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
        OutputFile f = std::make_shared<std::ofstream>();
        m_OutputFileMap.emplace(path, f);
        Open(f, path, append);
        return f;
    }
}

void FileDrainerIO::Open(InputFile &f, const std::string &path)
{

    f->rdbuf()->pubsetbuf(0, 0);
    f->open(path, std::ios::in | std::ios::binary);
}

void FileDrainerIO::Open(OutputFile &f, const std::string &path, bool append)
{

    if (append)
    {
        f->rdbuf()->pubsetbuf(0, 0);
        f->open(path, std::ios::out | std::ios::app | std::ios::binary);
    }
    else
    {
        f->rdbuf()->pubsetbuf(0, 0);
        f->open(path, std::ios::out | std::ios::trunc | std::ios::binary);
    }
}

void FileDrainerIO::Close(InputFile &f) { f->close(); }
void FileDrainerIO::Close(OutputFile &f) { f->close(); }

bool FileDrainerIO::Good(InputFile &f) { return (f->good()); }
bool FileDrainerIO::Good(OutputFile &f) { return (f->good()); }

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
    f->seekg(offset, std::ios_base::beg);
}

void FileDrainerIO::Seek(OutputFile &f, size_t offset, const std::string &path)
{
    f->seekp(offset, std::ios_base::beg);
}

void FileDrainerIO::SeekEnd(OutputFile &f) { f->seekp(0, std::ios_base::end); }

size_t FileDrainerIO::GetFileSize(InputFile &f)
{
    const auto currentOffset = f->tellg();
    f->seekg(0, std::ios_base::end);
    auto fileSize = f->tellg();
    f->seekg(currentOffset, std::ios_base::beg);
    return static_cast<size_t>(fileSize);
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
        const auto currentOffset = f->tellg();
        f->read(buffer, static_cast<std::streamsize>(count));
        const auto readSize = f->gcount();

        if (readSize < static_cast<std::streamsize>(count))
        {
            if (f->eof())
            {
                std::chrono::duration<double> d(sleepUnit);
                std::this_thread::sleep_for(d);
                f->clear(f->rdstate() & ~std::fstream::eofbit);
                totalSlept += sleepUnit;
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
        buffer += readSize;
        count -= readSize;
        totalRead += readSize;
    }
    return std::pair<size_t, double>(totalRead, totalSlept);
}

size_t FileDrainerIO::Write(OutputFile &f, size_t count, const char *buffer,
                            const std::string &path)
{
    f->write(buffer, static_cast<std::streamsize>(count));

    if (f->bad())
    {
        throw std::ios_base::failure(
            "FileDrainer couldn't write to file " + path +
            " count = " + std::to_string(count) + " bytes\n");
    }

    return count;
}

int FileDrainerIO::FileSync(OutputFile &f) { return 0; }

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
