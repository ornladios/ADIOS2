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
: DataManCommon("DataManReader", io, name, mode, mpiComm)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

DataManReader::~DataManReader()
{

    m_Listening = false;
    if (m_DataThread != nullptr)
    {
        m_DataThread->join();
    }
    if (m_ControlThread != nullptr)
    {
        m_ControlThread->join();
    }
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{
    ++m_CurrentStep;
    if (m_Synchronous == false)
    {
        m_CurrentStep = m_DataManDeserializer.MinStep();
    }
    StepStatus status;

    std::shared_ptr<std::vector<format::DataManDeserializer::DataManVar>> vars =
        m_DataManDeserializer.GetMetaData(m_CurrentStep);

    if (vars == nullptr)
    {
        status = StepStatus::NotReady;
    }
    else
    {
        for (const auto &i : *vars)
        {
            if (i.type == "compound")
            {
                throw("Compound type is not supported yet.");
            }
#define declare_type(T)                                                        \
    else if (i.type == GetType<T>())                                           \
    {                                                                          \
        adios2::Variable<T> *v = m_IO.InquireVariable<T>(i.name);              \
        if (v == nullptr)                                                      \
        {                                                                      \
            m_IO.DefineVariable<T>(i.name, i.shape, i.start, i.count);         \
        }                                                                      \
    }
            ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
        }
        status = StepStatus::OK;
    }
    return status;
}

void DataManReader::EndStep() { m_DataManDeserializer.Erase(m_CurrentStep); }

size_t DataManReader::CurrentStep() const { return m_CurrentStep; }

void DataManReader::PerformGets() {}

// PRIVATE

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

    GetBoolParameter(m_IO.m_Parameters, "Synchronous", m_Synchronous);
    GetStringParameter(m_IO.m_Parameters, "TransportMode", m_TransportMode);
    if (m_TransportMode == "subscribe")
    {
        m_Synchronous = false;
    }

    // initialize transports
    m_DataMan = std::make_shared<transportman::DataMan>(m_MPIComm, m_DebugMode);
    for (auto &i : m_IO.m_TransportsParameters)
    {
        i["TransportMode"] = m_TransportMode;
    }
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
    m_DataThread = std::make_shared<std::thread>(&DataManReader::IOThread, this,
                                                 m_DataMan);
}

void DataManReader::IOThread(std::shared_ptr<transportman::DataMan> man)
{
    while (m_Listening)
    {
        std::shared_ptr<std::vector<char>> buffer = man->ReadWAN();
        if (buffer != nullptr)
        {
            m_DataManDeserializer.Put(buffer);
        }
        if (m_Callbacks.empty() == false)
        {
            RunCallback();
        }
    }
}

void DataManReader::RunCallback()
{
    for (size_t step = m_DataManDeserializer.MinStep();
         step <= m_DataManDeserializer.MaxStep(); ++step)
    {
        std::vector<format::DataManDeserializer::DataManVar> varList;
        m_DataManDeserializer.GetVarList(step, varList);
        std::cout << varList[0].name << "  " << varList[0].type << std::endl;
    }

    for (auto &i : m_Callbacks)
    {
        // i->RunCallback2(buffer, doid, var, dtype, shape);
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
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void DataManReader::DoClose(const int transportIndex) {}

void DataManReader::IOThreadBP(std::shared_ptr<transportman::DataMan> man)
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

                std::memcpy(deserializer.m_Data.m_Buffer.data(), buffer->data(),
                            buffer->size());

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
                v->SetStepSelection({step.first - 1, 1});                      \
                deserializer.GetSyncVariableDataFromStream(                    \
                    *v, deserializer.m_Data);                                  \
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

} // end namespace adios2
