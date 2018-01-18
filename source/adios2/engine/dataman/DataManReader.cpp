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

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h" //CSVToVector

namespace adios2
{

DataManReader::DataManReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: Engine("DataManReader", io, name, mode, mpiComm), m_Man(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{

    StepStatus status;
    status = StepStatus::OK;

    status = StepStatus::EndOfStream;

    return status;
}

void DataManReader::ReadThread()
{

    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        while (m_Listening)
        {
            std::shared_ptr<std::vector<char>> buffer = m_Man.ReadWAN();
            if (buffer != nullptr)
            {
                if (buffer->size() > 0)
                {
                    std::shared_ptr<format::BP3Deserializer> deserializer =
                        std::make_shared<format::BP3Deserializer>(m_MPIComm,
                                                                  m_DebugMode);

                    deserializer->InitParameters(m_IO.m_Parameters);
                    deserializer->m_Data.Resize(
                        buffer->size(), "in DataMan Streaming Listener");

                    std::memcpy(deserializer->m_Data.m_Buffer.data(),
                                buffer->data(), buffer->size());

                    deserializer->ParseMetadata(deserializer->m_Data, m_IO);
                    const auto variablesInfo = m_IO.GetAvailableVariables();
                    for (const auto &variableInfoPair : variablesInfo)
                    {

                        std::string var = variableInfoPair.first;
                        std::string type = "null";

                        for (const auto &parameter : variableInfoPair.second)
                        {
                            if (parameter.first == "Type")
                            {
                                type = parameter.second;
                            }
                        }

                        std::shared_ptr<DataManVar> dmv =
                            std::make_shared<DataManVar>();

                        dmv->datatype = type;
                        dmv->deserializer = deserializer;

                        if (type == "compound")
                        {
                            throw("Compound type is not supported yet.");
                        }

#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        adios2::Variable<T> *v = m_IO.InquireVariable<T>(var);                 \
        deserializer->GetSyncVariableDataFromStream(*v, deserializer->m_Data); \
        if (v->GetData() == nullptr)                                           \
        {                                                                      \
            throw("Data pointer obtained from BP deserializer is anullptr");   \
        }                                                                      \
        else                                                                   \
        {                                                                      \
        }                                                                      \
    }
                        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
                    }
                }
            }
        }
    }
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
        m_IO.m_TransportsParameters[i]["Name"] = std::to_string(i);
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
            m_Callback = &j.ADIOSOperator;
            break;
        }
    }

    InitParameters();

    InitTransports();
}

void DataManReader::RunCallback(void *buffer, std::string doid, std::string var,
                                std::string dtype, std::vector<size_t> shape)
{
    if (m_Callback != nullptr && m_Callback->m_Type == "Signature2")
    {
        m_Callback->RunCallback2(buffer, doid, var, dtype, shape);
    }
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
