/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.cpp
 *
 *  Created on: Feb 21, 2017
 *      Author: Jason Wang
 *              William F Godoy
 */

#include "DataManReader.h"
#include "DataManReader.tcc"

#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManReader::DataManReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("DataManReader", io, name, mode, mpiComm),
  m_BP3Deserializer(mpiComm, m_DebugMode), m_Man(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{
    std::vector<char> buffer;
    buffer.reserve(m_BufferSize);
    size_t size = 0;

    m_Man.ReadWAN(buffer.data(), size);

    StepStatus status;

    if (size > 0)
    {
        status = StepStatus::OK;

        m_BP3Deserializer.m_Data.Resize(size, "in DataMan Streaming Listener");

        std::memcpy(m_BP3Deserializer.m_Data.m_Buffer.data(), buffer.data(),
                    size);

        m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Data, m_IO);
    }
    else
    {
        status = StepStatus::EndOfStream;
    }

    return status;
}

void DataManReader::PerformGets() {}

void DataManReader::EndStep() {}

void DataManReader::Close(const int transportIndex) {}

// PRIVATE
bool DataManReader::GetBoolParameter(Params &params, std::string key,
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

bool DataManReader::GetStringParameter(Params &params, std::string key,
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

bool DataManReader::GetUIntParameter(Params &params, std::string key,
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
void DataManReader::InitParameters()
{
    GetUIntParameter(m_IO.m_Parameters, "NChannels", m_NChannels);
    GetStringParameter(m_IO.m_Parameters, "Format", m_UseFormat);
}

void DataManReader::InitTransports()
{
    size_t channels = m_IO.m_TransportsParameters.size();
    std::vector<std::string> names;
    for (size_t i = 0; i < channels; ++i)
    {
        names.push_back(m_Name + std::to_string(i));
    }

    m_Man.OpenWANTransports(names, Mode::Read, m_IO.m_TransportsParameters,
                            true);
}
void DataManReader::Init()
{
    for (auto &j : m_IO.m_Operators)
    {
        if (j.ADIOSOperator.m_Type == "Signature2")
        {
            m_Man.SetCallback(j.ADIOSOperator);
            break;
        }
    }

    InitParameters();

    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        m_BP3Deserializer.InitParameters(m_IO.m_Parameters);
    }

    m_Man.SetBP3Deserializer(m_BP3Deserializer);
    m_Man.SetIO(m_IO);

    InitTransports();
}

#define declare_type(T)                                                        \
    void DataManReader::DoGetSync(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void DataManReader::DoGetDeferred(Variable<T> &variable, T *data)          \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    void DataManReader::DoGetDeferred(Variable<T> &variable, T &data)          \
    {                                                                          \
        GetDeferredCommon(variable, &data);                                    \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace adios2
