/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <ios> // std::ios_base::failure
#include <stdexcept>
/// \endcond

#include "transport/file/FStream.h"

namespace adios
{
namespace transport
{

FStream::FStream(MPI_Comm mpiComm, bool debugMode)
: Transport("fstream", mpiComm, debugMode)
{
}

void FStream::Open(const std::string name, const std::string accessMode)
{
    m_Name = name;
    m_AccessMode = accessMode;

    if (accessMode == "w" || accessMode == "write")
    {
        m_FStream.open(name, std::fstream::out);
    }
    else if (accessMode == "a" || accessMode == "append")
    {
        m_FStream.open(name, std::fstream::out | std::fstream::app);
    }
    else if (accessMode == "r" || accessMode == "read")
    {
        m_FStream.open(name, std::fstream::in);
    }

    if (m_DebugMode == true)
    {
        if (!m_FStream)
        {
            throw std::ios_base::failure(
                "ERROR: couldn't open file " + name +
                ", in call to Open from FStream transport\n");
        }
    }
}

void FStream::SetBuffer(char *buffer, std::size_t size)
{
    m_FStream.rdbuf()->pubsetbuf(buffer, size);
}

void FStream::Write(const char *buffer, std::size_t size)
{
    m_FStream.write(buffer, size);

    if (m_DebugMode == true)
    {
        if (!m_FStream)
        {
            throw std::ios_base::failure("ERROR: couldn't write to file " +
                                         m_Name +
                                         ", in call to FStream write\n");
        }
    }
}

void FStream::Flush() { m_FStream.flush(); }

void FStream::Close() { m_FStream.close(); }

} // end namespace transport
} // end namespace adios
