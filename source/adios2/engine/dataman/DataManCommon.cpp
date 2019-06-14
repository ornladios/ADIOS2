/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.cpp
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
  m_FileTransport(mpiComm, m_DebugMode)
{

    // initialize parameters
    MPI_Comm_rank(mpiComm, &m_MpiRank);
    MPI_Comm_size(mpiComm, &m_MpiSize);
    m_IsLittleEndian = helper::IsLittleEndian();
    m_IsRowMajor = helper::IsRowMajor(io.m_HostLanguage);
    GetStringParameter(m_IO.m_Parameters, "WorkflowMode", m_WorkflowMode);
    GetBoolParameter(m_IO.m_Parameters, "AlwaysProvideLatestTimestep",
                     m_ProvideLatest);
    if (m_WorkflowMode != "file" && m_WorkflowMode != "stream")
    {
        throw(std::invalid_argument(
            "WorkflowMode parameter for DataMan must be File or Stream"));
    }
    m_Channels = m_IO.m_TransportsParameters.size();
    if (m_Channels == 0)
    {
        m_Channels = 1;
        m_IO.m_TransportsParameters.push_back({{"Library", "ZMQ"},
                                               {"IPAddress", "127.0.0.1"},
                                               {"Port", "12306"},
                                               {"Name", m_Name}});
    }
    for (size_t i = 0; i < m_Channels; ++i)
    {
        m_IO.m_TransportsParameters[i]["Name"] = m_Name + std::to_string(i);
    }
}

bool DataManCommon::GetStringParameter(Params &params, std::string key,
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

bool DataManCommon::GetBoolParameter(Params &params, std::string key,
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
