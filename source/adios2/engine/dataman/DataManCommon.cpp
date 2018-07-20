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
: Engine(engineType, io, name, mode, mpiComm)
{

    MPI_Comm_rank(mpiComm, &m_MPIRank);
    MPI_Comm_size(mpiComm, &m_MPISize);

    m_IsLittleEndian = helper::IsLittleEndian();
    m_IsRowMajor = helper::IsRowMajor(io.m_HostLanguage);

    GetIntParameter(m_IO.m_Parameters, "TransportChannels",
                    m_TransportChannels);
}

bool DataManCommon::GetBoolParameter(Params &params, std::string key,
                                     bool &value)
{
    auto itKey = params.find(key);
    if (itKey != params.end())
    {
        std::transform(itKey->second.begin(), itKey->second.end(),
                       itKey->second.begin(), ::tolower);
        if (itKey->second == "yes" || itKey->second == "true")
        {
            value = true;
            return true;
        }
        if (itKey->second == "no" || itKey->second == "false")
        {
            value = false;
            return true;
        }
    }
    return false;
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

bool DataManCommon::GetIntParameter(Params &params, std::string key, int &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = std::stoi(it->second);
        return true;
    }
    return false;
}

std::vector<std::string> DataManCommon::ParseAddress(std::string input,
                                                     std::string protocol)
{
    std::vector<std::string> strings;
    std::istringstream f(input);
    std::string s;
    while (getline(f, s, ','))
    {
        s.erase(remove_if(s.begin(), s.end(), isspace), s.end());
        if (protocol == "tcp")
        {
            s = protocol + "://" + s;
        }
        strings.push_back(s);
    }
    return std::move(strings);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
