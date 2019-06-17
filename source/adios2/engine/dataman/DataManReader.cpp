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

#include "adios2/common/ADIOSMacros.h"
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
  m_DataManSerializer(m_IsRowMajor, m_ContiguousMajor, m_IsLittleEndian,
                      mpiComm)
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
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::BeginStep() begin. Last step "
                  << m_CurrentStep << std::endl;
    }

    if (m_CurrentStep == m_FinalStep && m_CurrentStep > 0)
    {
        return StepStatus::EndOfStream;
    }

    format::DmvVecPtr vars = nullptr;
    auto start_time = std::chrono::system_clock::now();

    while (vars == nullptr)
    {
        auto now_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(
            now_time - start_time);
        // timeout == std::numeric_limits<float>::max() means there is no
        // timeout, and it should block
        // forever until it receives something.
        if (timeoutSeconds >= 0.0)
        {
            if (duration.count() > timeoutSeconds)
            {
                return StepStatus::NotReady;
            }
        }

        m_MetaDataMap = m_DataManSerializer.GetMetaData();

        if (!m_ProvideLatest)
        {
            size_t minStep = std::numeric_limits<size_t>::max();
            ;
            for (const auto &i : m_MetaDataMap)
            {
                if (minStep > i.first)
                {
                    minStep = i.first;
                }
            }
            m_CurrentStep = minStep;
        }
        else
        {
            size_t maxStep = 0;
            for (const auto &i : m_MetaDataMap)
            {
                if (maxStep < i.first)
                {
                    maxStep = i.first;
                }
            }
            m_CurrentStep = maxStep;
        }

        auto currentStepIt = m_MetaDataMap.find(m_CurrentStep);
        if (currentStepIt != m_MetaDataMap.end())
        {
            vars = currentStepIt->second;
        }
    }

    m_DataManSerializer.GetAttributes(m_IO);

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
            ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
            else
            {
                throw("Unknown type caught in "
                      "DataManReader::BeginStepSubscribe.");
            }
        }
    }

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::BeginStep() end. Current step "
                  << m_CurrentStep << std::endl;
    }

    return StepStatus::OK;
}

size_t DataManReader::CurrentStep() const { return m_CurrentStep; }

void DataManReader::PerformGets() {}

void DataManReader::EndStep()
{

    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::EndStep() start. Current step "
                  << m_CurrentStep << std::endl;
    }
    m_DataManSerializer.Erase(m_CurrentStep);
    if (m_Verbosity >= 5)
    {
        std::cout << "DataManReader::EndStep() end. Current step "
                  << m_CurrentStep << std::endl;
    }
}

void DataManReader::Flush(const int transportIndex) {}

// PRIVATE

void DataManReader::Init()
{
    if (m_WorkflowMode == "file")
    {
        m_FileTransport.Open(m_Name, Mode::Read);
        return;
    }

    // initialize transports
    m_WANMan = std::make_shared<transportman::WANMan>(m_MPIComm, m_DebugMode);
    m_WANMan->OpenTransports(m_IO.m_TransportsParameters, Mode::Read,
                             m_WorkflowMode, true);

    // start threads
    m_Listening = true;
    m_DataThread =
        std::make_shared<std::thread>(&DataManReader::IOThread, this, m_WANMan);
}

void DataManReader::IOThread(std::shared_ptr<transportman::WANMan> man)
{
    while (m_Listening)
    {
        std::shared_ptr<std::vector<char>> buffer = man->Read(0);
        if (buffer != nullptr)
        {
            int ret = m_DataManSerializer.PutPack(buffer);
            if (ret > 0)
            {
                m_FinalStep = ret;
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
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void DataManReader::DoClose(const int transportIndex)
{
    if (transportIndex == -1)
    {
        m_Listening = false;
        if (m_DataThread != nullptr)
        {
            if (m_DataThread->joinable())
            {
                m_DataThread->join();
            }
        }
    }
    m_WANMan = nullptr;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
