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
: Engine("DataManReader", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

DataManReader::~DataManReader()
{

    m_Listening = false;
    for (auto &t : m_DataThreads)
    {
        t.join();
    }
    for (auto &t : m_ControlThreads)
    {
        t.join();
    }
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{

    StepStatus status;
    status = StepStatus::OK;

    status = StepStatus::EndOfStream;

    return status;
}

void DataManReader::ReadThread(std::shared_ptr<transportman::DataMan> man)
{

    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        format::BP3Deserializer deserializer(m_MPIComm, m_DebugMode);

        deserializer.InitParameters(m_IO.m_Parameters);

        while (m_Listening)
        {
            // TODO: This is absolutely not thread safe. If future
            // implementation needs multiple threads then this has to be
            // rewriten by manually removing variables one by one and it
            // has to be protected by mutex.
            //                    m_IO.RemoveAllVariables();
            std::shared_ptr<std::vector<char>> buffer = man->ReadWAN();
            if (buffer != nullptr)
            {
                if (buffer->size() > 0)
                {
                    deserializer.m_Data.Resize(buffer->size(),
                                               "in DataMan Streaming Listener");

                    std::memcpy(deserializer.m_Data.m_Buffer.data(),
                                buffer->data(), buffer->size());

                    m_MutexIO.lock();
                    deserializer.ParseMetadata(deserializer.m_Data, m_IO);
                    m_MutexIO.unlock();

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

                        if (type == "compound")
                        {
                            throw("Compound type is not supported yet.");
                        }

// for debug
/*
else if (type == GetType<float>())
{
adios2::Variable<float> *v =
m_IO.InquireVariable<float>(var);
deserializer.GetSyncVariableDataFromStream(
*v, deserializer.m_Data);
if (v->GetData() == nullptr)
{
throw("Data pointer obtained from BP "
"deserializer is anullptr");
}
else
{
for (auto &step : v->m_IndexStepBlockStarts)
{
std::shared_ptr<DataManVar> dmv =
std::make_shared<DataManVar>();
dmv->datatype = type;
dmv->shape = v->m_Shape;
dmv->start = v->m_Start;
dmv->count = v->m_Count;
dmv->data.resize(v->PayloadSize());
v->SetStepSelection({step.first - 1, 1});
deserializer.GetSyncVariableDataFromStream(
*v, deserializer.m_Data);
std::memcpy(dmv->data.data(), v->GetData(),
v->PayloadSize());
RunCallback(v->GetData(), "stream", var,
type, v->m_Shape);
m_VariableMap[step.first - 1][var] = dmv;
}
}
}
*/

#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        adios2::Variable<T> *v = m_IO.InquireVariable<T>(var);                 \
        deserializer.GetSyncVariableDataFromStream(*v, deserializer.m_Data);   \
        if (v->GetData() == nullptr)                                           \
        {                                                                      \
            throw("Data pointer obtained from BP "                             \
                  "deserializer is anullptr");                                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            for (auto &step : v->m_IndexStepBlockStarts)                       \
            {                                                                  \
                std::shared_ptr<DataManVar> dmv =                              \
                    std::make_shared<DataManVar>();                            \
                dmv->datatype = type;                                          \
                dmv->shape = v->m_Shape;                                       \
                dmv->start = v->m_Start;                                       \
                dmv->count = v->m_Count;                                       \
                dmv->data.resize(v->PayloadSize());                            \
                v->SetStepSelection({step.first - 1, 1});                      \
                deserializer.GetSyncVariableDataFromStream(                    \
                    *v, deserializer.m_Data);                                  \
                std::memcpy(dmv->data.data(), v->GetData(), v->PayloadSize()); \
                RunCallback(v->GetData(), "stream", var, type, v->m_Shape);    \
                m_VariableMap[step.first - 1][var] = dmv;                      \
            }                                                                  \
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

void DataManReader::EndStep() { ++m_CurrentStep; }

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

void DataManReader::Init()
{

    // register callbacks
    for (auto &j : m_IO.m_Operators)
    {
        if (j.ADIOSOperator.m_Type == "Signature2")
        {
            m_Callbacks.push_back(&j.ADIOSOperator);
        }
    }

    // get parameters
    GetUIntParameter(m_IO.m_Parameters, "NChannels", m_NChannels);
    GetStringParameter(m_IO.m_Parameters, "Format", m_UseFormat);

    // initialize transports

    m_DataMan = std::make_shared<transportman::DataMan>(m_MPIComm, m_DebugMode);

    size_t channels = m_IO.m_TransportsParameters.size();
    std::vector<std::string> names;
    for (size_t i = 0; i < channels; ++i)
    {
        names.push_back(m_Name + std::to_string(i));
        m_IO.m_TransportsParameters[i]["Name"] = std::to_string(i);
    }

    m_DataMan->OpenWANTransports(names, Mode::Read, m_IO.m_TransportsParameters,
                                 true);

    // start threads

    m_Listening = true;
    for (size_t i = 0; i < m_DataChannels; ++i)
    {
        m_DataThreads.emplace_back(
            std::thread(&DataManReader::ReadThread, this, m_DataMan));
    }
    for (size_t i = 0; i < m_ControlChannels; ++i)
    {
        m_ControlThreads.emplace_back(
            std::thread(&DataManReader::ReadThread, this, m_ControlMan));
    }
}

void DataManReader::RunCallback(void *buffer, std::string doid, std::string var,
                                std::string dtype, std::vector<size_t> shape)
{
    for (auto &i : m_Callbacks)
    {
        if (i != nullptr)
        {
            if (i->m_Type == "Signature2")
            {
                i->RunCallback2(buffer, doid, var, dtype, shape);
            }
        }
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

void DataManReader::DoClose(const int transportIndex) {}

} // end namespace adios2
