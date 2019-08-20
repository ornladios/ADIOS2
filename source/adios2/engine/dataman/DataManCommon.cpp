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
                             MPI_Comm mpiComm)
: Engine(engineType, io, name, mode, mpiComm),
  m_IsRowMajor(helper::IsRowMajor(io.m_HostLanguage)),
  m_DataManSerializer(mpiComm, m_IsRowMajor)
{
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    MPI_Comm_size(mpiComm, &m_MpiSize);
    GetParameter(m_IO.m_Parameters, "IPAddress", m_IPAddress);
    GetParameter(m_IO.m_Parameters, "Port", m_Port);
    GetParameter(m_IO.m_Parameters, "StagingMode", m_StagingMode);
    GetParameter(m_IO.m_Parameters, "Timeout", m_Timeout);
    GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);
}

DataManCommon::~DataManCommon() {}

bool DataManCommon::GetParameter(const Params &params, const std::string &key,
                                 std::string &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = it->second;
        std::transform(value.begin(), value.end(), value.begin(), ::tolower);
        return true;
    }
    return false;
}

bool DataManCommon::GetParameter(const Params &params, const std::string &key,
                                 int &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = stoi(it->second);
        return true;
    }
    return false;
}

bool DataManCommon::GetParameter(const Params &params, const std::string &key,
                                 bool &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        if (it->second == "yes" || it->second == "true")
        {
            value = true;
        }
        else if (it->second == "no" || it->second == "false")
        {
            value = false;
        }
        return true;
    }
    return false;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
