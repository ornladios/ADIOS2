/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManCommon.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: Jason Wang
 */

#include "DataManCommon.h"

namespace adios2
{
namespace core
{
namespace engine
{

DataManCommon::DataManCommon(const std::string engineType, IO &io,
                             const std::string &name, const Mode mode,
                             helper::Comm comm)
: Engine(engineType, io, name, mode, std::move(comm)),
  m_IsRowMajor(helper::IsRowMajor(io.m_HostLanguage)),
  m_FastSerializer(m_Comm, m_IsRowMajor)
{
    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();
    helper::GetParameter(m_IO.m_Parameters, "IPAddress", m_IPAddress);
    helper::GetParameter(m_IO.m_Parameters, "Port", m_Port);
    helper::GetParameter(m_IO.m_Parameters, "Timeout", m_Timeout);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
    helper::GetParameter(m_IO.m_Parameters, "RendezvousReaderCount",
                         m_RendezvousReaderCount);
    helper::GetParameter(m_IO.m_Parameters, "RendezvousMilliseconds",
                         m_RendezvousMilliseconds);
}

DataManCommon::~DataManCommon() {}

} // end namespace engine
} // end namespace core
} // end namespace adios2
