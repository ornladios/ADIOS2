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
namespace core
{
namespace engine
{

DataManReader::DataManReader(IO &io, const std::string &name, const Mode mode,
                             MPI_Comm mpiComm)
: DataManCommon("DataManReader", io, name, mode, mpiComm),
  m_DataManDeserializer(m_IsRowMajor, m_IsLittleEndian)
{
    m_EndMessage = " in call to IO Open DataManReader " + m_Name + "\n";
    Init();
}

DataManReader::~DataManReader()
{
    if (m_IsClosed == false)
    {
        DoClose();
    }
}

StepStatus DataManReader::BeginStep(StepMode stepMode,
                                    const float timeoutSeconds)
{
    if (m_CurrentStep == m_FinalStep && m_CurrentStep > 0 && m_FinalStep > 0)
    {
        return StepStatus::EndOfStream;
    }
    if (m_WorkflowMode == "subscribe")
    {
        return BeginStepSubscribe(stepMode, timeoutSeconds);
    }
    else if (m_WorkflowMode == "p2p")
    {
        return BeginStepP2P(stepMode, timeoutSeconds);
    }
    else
    {
        throw(std::invalid_argument(
            "[DataManReader::BeginStep] invalid workflow mode " +
            m_WorkflowMode));
        return StepStatus::EndOfStream;
    }
}

size_t DataManReader::CurrentStep() const { return m_CurrentStep; }

void DataManReader::PerformGets() {}

void DataManReader::EndStep() { m_DataManDeserializer.Erase(m_CurrentStep); }

void DataManReader::Flush(const int transportIndex) {}

// PRIVATE

void DataManReader::Init()
{
    // initialize transports
    m_DataMan = std::make_shared<transportman::DataMan>(m_MPIComm, m_DebugMode);
    m_DataMan->OpenWANTransports(m_StreamNames, m_IO.m_TransportsParameters,
                                 Mode::Read, m_WorkflowMode, true);

    // start threads
    m_Listening = true;
    m_DataThread = std::make_shared<std::thread>(&DataManReader::IOThread, this,
                                                 m_DataMan);
}

StepStatus DataManReader::BeginStepSubscribe(StepMode stepMode,
                                             const float timeoutSeconds)
{
    StepStatus status;
    m_CurrentStep = m_DataManDeserializer.MinStep();
    m_MetaDataMap = m_DataManDeserializer.GetMetaData();
    auto vars = m_MetaDataMap[m_CurrentStep];
    if (vars == nullptr)
    {
        status = StepStatus::NotReady;
    }
    else
    {
        for (const auto &i : *vars)
        {
            if (i.step == m_CurrentStep)
            {
                if (i.type == "compound")
                {
                    throw("Compound type is not supported yet.");
                }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
    }
                ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
                else
                {
                    throw("Unknown type caught in "
                          "DataManReader::BegineStepSubscribe.");
                }
            }
        }
        status = StepStatus::OK;
    }
    return status;
}

StepStatus DataManReader::BeginStepP2P(StepMode stepMode,
                                       const float timeoutSeconds)
{
    ++m_CurrentStep;

    std::shared_ptr<std::vector<format::DataManDeserializer::DataManVar>> vars =
        nullptr;

    auto start_time = std::chrono::system_clock::now();

    while (vars == nullptr)
    {
        // Copy the metadata map locally. This map is light in memory because
        // the var metadata is stored in shared objects. Once this is copied,
        // even if the map is changed in deserializer, these shared objects are
        // still valid.
        m_MetaDataMap = m_DataManDeserializer.GetMetaData();
        auto i = m_MetaDataMap.find(m_CurrentStep);
        if (i == m_MetaDataMap.end())
        {
            auto now_time = std::chrono::system_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                now_time - start_time);
            // timeout == 0.f means there is no timeout, and it should block
            // forever until it receives something.
            if (duration.count() > timeoutSeconds && timeoutSeconds != 0.f)
            {
                return StepStatus::EndOfStream;
            }
            // sleep for a little time because
            // m_DataManDeserializer.GetMetaData(m_CurrentStep) will lock the
            // metadata map in deserializer. If this thread is always locking
            // the map then it would delay the IOThread from putting new
            // variables into the map.
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        else
        {
            vars = i->second;
        }
    }

    // register all variables available in the current step into m_IO
    for (const auto &i : *vars)
    {
        if (i.type == "compound")
        {
            throw("Compound type is not supported yet.");
        }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    return StepStatus::OK;
}

void DataManReader::IOThread(std::shared_ptr<transportman::DataMan> man)
{
    while (m_Listening)
    {
        std::shared_ptr<std::vector<char>> buffer = man->ReadWAN(0);
        if (buffer != nullptr)
        {
            int ret = m_DataManDeserializer.Put(buffer);
            if (ret > 0)
            {
                m_FinalStep = ret;
            }
        }
        RunCallback();
    }
}

void DataManReader::RunCallback()
{
    std::lock_guard<std::mutex> l(m_CallbackMutex);
    if (m_Callbacks.empty() == false)
    {
        for (size_t step = m_DataManDeserializer.MinStep();
             step <= m_DataManDeserializer.MaxStep(); ++step)
        {
            auto varList = m_DataManDeserializer.GetMetaData(step);
            if (varList == nullptr)
            {
                continue;
            }
            for (const auto &i : *varList)
            {
                if (i.type == "compound")
                {
                    throw("Compound type is not supported yet.");
                }
#define declare_type(T)                                                        \
    else if (i.type == helper::GetType<T>())                                   \
    {                                                                          \
        CheckIOVariable<T>(i.name, i.shape, i.start, i.count);                 \
        size_t datasize =                                                      \
            std::accumulate(i.count.begin(), i.count.end(), sizeof(T),         \
                            std::multiplies<size_t>());                        \
        std::vector<T> varData(datasize, std::numeric_limits<T>::quiet_NaN()); \
        m_DataManDeserializer.Get(varData.data(), i.name, i.start, i.count,    \
                                  step);                                       \
        for (auto &j : m_Callbacks)                                            \
        {                                                                      \
            if (j->m_Type == "Signature1")                                     \
            {                                                                  \
                j->RunCallback2(reinterpret_cast<T *>(varData.data()), i.doid, \
                                i.name, i.type, step, i.shape, i.start,        \
                                i.count);                                      \
            }                                                                  \
            else if (j->m_Type == "Signature2")                                \
            {                                                                  \
                j->RunCallback2(varData.data(), i.doid, i.name, i.type, step,  \
                                i.shape, i.start, i.count);                    \
            }                                                                  \
            else                                                               \
            {                                                                  \
                throw(std::runtime_error(                                      \
                    "[DataManReader::RunCallback] Callback funtion "           \
                    "registered is not of Signatrue2. It might be modified "   \
                    "from outside DataMan Engine."));                          \
            }                                                                  \
        }                                                                      \
    }
                ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
            }
            m_DataManDeserializer.Erase(step);
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
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    DataManReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const     \
    {                                                                          \
        return AllStepsBlocksInfoCommon(variable);                             \
    }                                                                          \
    std::vector<typename Variable<T>::Info> DataManReader::DoBlocksInfo(       \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return BlocksInfoCommon(variable, step);                               \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void DataManReader::DoClose(const int transportIndex)
{
    if (transportIndex == -1)
    {
        m_Listening = false;
        m_CallbackMutex.lock();
        m_Callbacks.resize(0);
        m_CallbackMutex.unlock();
        if (m_DataThread != nullptr)
        {
            m_DataThread->join();
        }
    }
    m_DataMan = nullptr;
}

void DataManReader::IOThreadBP(std::shared_ptr<transportman::DataMan> man)
{
    format::BP3Deserializer deserializer(m_MPIComm, m_DebugMode);
    deserializer.InitParameters(m_IO.m_Parameters);

    while (m_Listening)
    {
        std::shared_ptr<std::vector<char>> buffer = man->ReadWAN(0);
        if (buffer != nullptr)
        {
            if (buffer->size() > 0)
            {
                deserializer.m_Data.Resize(buffer->size(),
                                           "in DataMan Streaming Listener");

                std::memcpy(deserializer.m_Data.m_Buffer.data(), buffer->data(),
                            buffer->size());

                m_IO.RemoveAllVariables();
                m_IO.RemoveAllAttributes();
                deserializer.ParseMetadata(deserializer.m_Data, m_IO);

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
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *v = m_IO.InquireVariable<T>(var);                         \
        if (v == nullptr)                                                      \
        {                                                                      \
            throw std::runtime_error("Data pointer obtained from BP "          \
                                     "deserializer is anullptr");              \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            for (auto &step : v->m_AvailableStepBlockIndexOffsets)             \
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

} // end namespace engine
} // end namespace core
} // end namespace adios2
