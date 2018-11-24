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
    MPI_Comm_rank(mpiComm, &m_MPIRank);
    MPI_Comm_size(mpiComm, &m_MPISize);
    m_IsLittleEndian = helper::IsLittleEndian();
    m_IsRowMajor = helper::IsRowMajor(io.m_HostLanguage);
    GetStringParameter(m_IO.m_Parameters, "WorkflowMode", m_WorkflowMode);
    GetStringParameter(m_IO.m_Parameters, "Format", m_Format);
    m_TransportChannels = m_IO.m_TransportsParameters.size();
    if (m_TransportChannels == 0)
    {
        m_TransportChannels = 1;
        m_IO.m_TransportsParameters.push_back({{"Library", "ZMQ"},
                                               {"IPAddress", "127.0.0.1"},
                                               {"Port", "12306"}});
    }
    for (size_t i = 0; i < m_TransportChannels; ++i)
    {
        m_StreamNames.push_back(m_Name + std::to_string(i));
    }

    // register callbacks
    for (auto &j : m_IO.m_Operations)
    {
        if (j.Op->m_Type == "Signature2")
        {
            m_Callbacks.push_back(j.Op);
        }
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

} // end namespace engine
} // end namespace core
} // end namespace adios2
