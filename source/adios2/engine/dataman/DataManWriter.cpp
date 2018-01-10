/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.cpp
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#include "DataManWriter.h"
#include "DataManWriter.tcc"

#include <iostream> //needs to go away, this is just for demo purposes

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManWriter::DataManWriter(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("DataManWriter", io, name, mode, mpiComm),
  m_BP3Serializer(mpiComm, m_DebugMode), m_Man(mpiComm, m_DebugMode),
  m_Name(name)
{
    m_EndMessage = ", in call to Open DataManWriter\n";
    Init();
}

StepStatus DataManWriter::BeginStep(StepMode mode, const float timeout_sec)
{
    return StepStatus::OK;
}
void DataManWriter::EndStep()
{
    if (m_UseFormat == "bp" || m_UseFormat == "BP")
    {
        m_BP3Serializer.SerializeData(m_IO, true);
    }
}

void DataManWriter::Close(const int transportIndex)
{
    if (m_UseFormat == "bp" || m_UseFormat == "BP")
    {
        m_BP3Serializer.CloseData(m_IO);
        auto &buffer = m_BP3Serializer.m_Data.m_Buffer;
        auto &position = m_BP3Serializer.m_Data.m_Position;
        if (position > 0)
        {
            m_Man.WriteWAN(buffer.data(), position);
        }
    }
}

// PRIVATE functions below

bool DataManWriter::GetBoolParameter(Params &params, std::string key,
                                     bool &value)
{
    auto itKey = params.find(key);
    if (itKey != params.end())
    {
        if (itKey->second == "yes" || itKey->second == "YES" ||
            itKey->second == "Yes" || itKey->second == "true" ||
            itKey->second == "TRUE" || itKey->second == "True")
        {
            value = true;
            return true;
        }
        if (itKey->second == "no" || itKey->second == "NO" ||
            itKey->second == "No" || itKey->second == "false" ||
            itKey->second == "FALSE" || itKey->second == "False")
        {
            value = false;
            return true;
        }
    }
    return false;
}

bool DataManWriter::GetStringParameter(Params &params, std::string key,
                                       std::string &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = it->second;
        return true;
    }
    return false;
}

bool DataManWriter::GetUIntParameter(Params &params, std::string key,
                                     unsigned int &value)
{
    auto it = params.find(key);
    if (it != params.end())
    {
        value = std::stoi(it->second);
        return true;
    }
    return false;
}

void DataManWriter::InitParameters()
{

    GetBoolParameter(m_IO.m_Parameters, "Monitoring", m_DoMonitor);
    GetUIntParameter(m_IO.m_Parameters, "NTransports", m_NChannels);

    // Check if using BP Format and initialize buffer
    GetStringParameter(m_IO.m_Parameters, "Format", m_UseFormat);
    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        m_BP3Serializer.InitParameters(m_IO.m_Parameters);
        m_BP3Serializer.PutProcessGroupIndex(m_IO.m_HostLanguage, {"WAN_Zmq"});
    }
}

void DataManWriter::InitTransports()
{

    size_t channels = m_IO.m_TransportsParameters.size();
    std::vector<std::string> names;
    for (size_t i = 0; i < channels; ++i)
    {
        names.push_back(m_Name + std::to_string(i));
    }

    m_Man.OpenWANTransports(names, Mode::Write, m_IO.m_TransportsParameters,
                            true);
}

void DataManWriter::Init()
{
    InitParameters();
    InitTransports();
}

#define declare_type(T)                                                        \
    void DataManWriter::DoPutSync(Variable<T> &variable, const T *values)      \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void DataManWriter::DoPutDeferred(Variable<T> &variable, const T *values)  \
    {                                                                          \
        PutDeferredCommon(variable, values);                                   \
    }                                                                          \
    void DataManWriter::DoPutDeferred(Variable<T> &, const T &value) {}
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
