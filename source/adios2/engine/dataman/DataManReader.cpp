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

    m_MutexMap.lock();
    bool vmap_empty = m_VariableMap.empty();
    m_MutexMap.unlock();

    if (vmap_empty)
    {
        status = StepStatus::NotReady;
    }
    else
    {
        status = StepStatus::OK;
    }

    if (!m_Blocking)
    {
        m_OldestStep = 0xffffff;
        size_t p_old = m_OldestStep;
        for (auto &i : m_VariableMap)
        {
            if (i.first < m_OldestStep)
            {
                m_OldestStep = i.first;
            }
        }
        if (p_old == m_OldestStep)
        {
            status = StepStatus::NotReady;
        }
        else if (p_old > m_OldestStep)
        {
            status = StepStatus::OK;
        }
    }

    return status;
}

size_t DataManReader::CurrentStep() const { return m_CurrentStep; }

void DataManReader::EndStep()
{
    // delete any time steps older than the current step
    for (int m = m_OldestStep; m <= m_CurrentStep; ++m)
    {
        m_MutexMap.lock();
        auto k = m_VariableMap.find(m);
        m_MutexMap.unlock();
        if (k != m_VariableMap.end())
        {
            m_MutexMap.lock();
            m_VariableMap.erase(k);
            m_MutexMap.unlock();
        }
    }

    if (m_Blocking)
    {
        m_OldestStep = m_CurrentStep;
        ++m_CurrentStep;
    }
}

void DataManReader::ReadThread(std::shared_ptr<transportman::DataMan> man)
{

    if (m_UseFormat == "BP" || m_UseFormat == "bp")
    {
        format::BP3Deserializer deserializer(m_MPIComm, m_DebugMode);
        deserializer.InitParameters(m_IO.m_Parameters);

        while (m_Listening)
        {
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
                    m_IO.RemoveAllVariables();
                    m_IO.RemoveAllAttributes();
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

#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        adios2::Variable<T> *v = m_IO.InquireVariable<T>(var);                 \
        if (v == nullptr)                                                      \
        {                                                                      \
            throw std::runtime_error("Data pointer obtained from BP "          \
                                     "deserializer is anullptr");              \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            for (auto &step : v->m_IndexStepBlockStarts)                       \
            {                                                                  \
                std::shared_ptr<DataManVar> dmv =                              \
                    std::make_shared<DataManVar>();                            \
                dmv->type = type;                                              \
                dmv->shape = v->m_Shape;                                       \
                dmv->start = v->m_Start;                                       \
                dmv->count = v->m_Count;                                       \
                dmv->data.resize(v->PayloadSize());                            \
                v->SetStepSelection({step.first - 1, 1});                      \
                deserializer.GetSyncVariableDataFromStream(                    \
                    *v, deserializer.m_Data);                                  \
                std::memcpy(dmv->data.data(), v->GetData(), v->PayloadSize()); \
                RunCallback(v->GetData(), "stream", var, type, v->m_Shape);    \
                m_MutexMap.lock();                                             \
                m_VariableMap[step.first - 1][var] = dmv;                      \
                m_MutexMap.unlock();                                           \
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

    else if (m_UseFormat == "json")
    {

        while (m_Listening)
        {
            std::shared_ptr<std::vector<char>> buffer = man->ReadWAN();
            if (buffer != nullptr)
            {
                size_t flagsize = sizeof(size_t);
                if (buffer->size() > flagsize)
                {
                    size_t metasize;
                    std::memcpy(&metasize, buffer->data(), flagsize);

                    if (metasize > 0)
                    {
                        char metastr[metasize + 1];
                        std::memcpy(metastr, buffer->data() + flagsize,
                                    metasize);
                        metastr[metasize] = '\0';
                        nlohmann::json metaj = nlohmann::json::parse(metastr);

                        std::shared_ptr<DataManVar> dmv =
                            std::make_shared<DataManVar>();

                        dmv->shape = metaj["S"].get<std::vector<size_t>>();
                        dmv->count = metaj["C"].get<std::vector<size_t>>();
                        dmv->start = metaj["O"].get<std::vector<size_t>>();
                        size_t step = metaj["T"].get<size_t>();
                        std::string name = metaj["N"].get<std::string>();
                        dmv->type = metaj["Y"].get<std::string>();
                        dmv->size = metaj["I"].get<size_t>();

                        if (dmv->type == "compound")
                        {
                            throw("Compound type is not supported yet.");
                        }
#define declare_type(T)                                                        \
    else if (dmv->type == GetType<T>())                                        \
    {                                                                          \
        adios2::Variable<T> *v = m_IO.InquireVariable<T>(name);                \
        if (v == nullptr)                                                      \
        {                                                                      \
            m_IO.DefineVariable<T>(name, dmv->shape, dmv->start, dmv->count);  \
        }                                                                      \
        else                                                                   \
        {                                                                      \
        }                                                                      \
    }
                        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

                        dmv->data.resize(metaj["I"].get<size_t>());
                        std::memcpy(dmv->data.data(),
                                    buffer->data() + flagsize + metasize,
                                    dmv->size);

                        m_MutexMap.lock();
                        m_VariableMap[step][name] = dmv;
                        /*
                        if (!m_Blocking)
                        {
                            if (m_CurrentStep == step)
                            {
                                ++m_CurrentStep;
                            }
                            else
                            {
                                m_CurrentStep = step;
                            }
                        }
                        */
                        m_MutexMap.unlock();
                        RunCallback(dmv->data.data(), "stream", name, dmv->type,
                                    dmv->shape);
                    }
                }
            }
        }
    }
}

void DataManReader::PerformGets() {}

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
    GetUIntParameter(m_IO.m_Parameters, "DataThreads", m_nDataThreads);
    GetUIntParameter(m_IO.m_Parameters, "ControlThreads", m_nControlThreads);
    GetUIntParameter(m_IO.m_Parameters, "TransportChannels",
                     m_TransportChannels);
    GetBoolParameter(m_IO.m_Parameters, "Blocking", m_Blocking);
    //    GetStringParameter(m_IO.m_Parameters, "Format", m_UseFormat);

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
                                 true, m_Blocking);

    // start threads

    m_Listening = true;
    for (size_t i = 0; i < m_nDataThreads; ++i)
    {
        m_DataThreads.emplace_back(
            std::thread(&DataManReader::ReadThread, this, m_DataMan));
    }
    for (size_t i = 0; i < m_nControlThreads; ++i)
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
