/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Reader.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Reader.h"
#include "BP4Reader.tcc"

#include "adios2/helper/adiosFunctions.h" // MPI BroadcastVector
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

namespace adios2
{
namespace core
{
namespace engine
{

BP4Reader::BP4Reader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("BP4Reader", io, name, mode, mpiComm),
  m_BP4Deserializer(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode),
  m_SubFileManager(mpiComm, m_DebugMode),
  m_FileMetadataIndexManager(mpiComm, m_DebugMode)
{
    TAU_SCOPED_TIMER("BP4Reader::Open");
    Init();
}

StepStatus BP4Reader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER("BP4Reader::BeginStep");
    if (m_DebugMode)
    {
        if (mode != StepMode::Read)
        {
            throw std::invalid_argument("ERROR: mode is not supported yet, "
                                        "only Read is valid for "
                                        "engine BP4Reader, in call to "
                                        "BeginStep\n");
        }

        if (!m_BP4Deserializer.m_DeferredVariables.empty())
        {
            throw std::invalid_argument(
                "ERROR: existing variables subscribed with "
                "GetDeferred, did you forget to call "
                "PerformGets() or EndStep()?, in call to BeginStep\n");
        }
    }

    if (m_FirstStep)
    {
        m_FirstStep = false;
    }
    else
    {
        ++m_CurrentStep;
    }

    // used to inquire for variables in streaming mode
    m_IO.m_ReadStreaming = true;
    m_IO.m_EngineStep = m_CurrentStep;

    if (m_CurrentStep >= m_BP4Deserializer.m_MetadataSet.StepsCount)
    {
        m_IO.m_ReadStreaming = false;
        return StepStatus::EndOfStream;
    }

    /*
    const auto &variablesData = m_IO.GetVariablesDataMap();

    for (const auto &variableData : variablesData)
    {
        const std::string name = variableData.first;
        const std::string type = m_IO.InquireVariableType(name);

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(name);                 \
        if (mode == StepMode::NextAvailable)                                   \
        {                                                                      \
            variable->SetStepSelection({m_CurrentStep, 1});                    \
        }                                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
    */
    m_IO.ResetVariablesStepSelection(false, "in call to BP4 Reader BeginStep");

    return StepStatus::OK;
}

size_t BP4Reader::CurrentStep() const { return m_CurrentStep; }

void BP4Reader::EndStep()
{
    TAU_SCOPED_TIMER("BP4Reader::EndStep");
    PerformGets();
}

void BP4Reader::PerformGets()
{
    TAU_SCOPED_TIMER("BP4Reader::PerformGets");
    if (m_BP4Deserializer.m_DeferredVariables.empty())
    {
        return;
    }

    for (const std::string &name : m_BP4Deserializer.m_DeferredVariables)
    {
        const std::string type = m_IO.InquireVariableType(name);

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable =                                                \
            FindVariable<T>(name, "in call to PerformGets, EndStep or Close"); \
        for (auto &blockInfo : variable.m_BlocksInfo)                          \
        {                                                                      \
            m_BP4Deserializer.SetVariableBlockInfo(variable, blockInfo);       \
        }                                                                      \
        ReadVariableBlocks(variable);                                          \
        variable.m_BlocksInfo.clear();                                         \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    m_BP4Deserializer.m_DeferredVariables.clear();
}

// PRIVATE
void BP4Reader::Init()
{
    if (m_DebugMode)
    {
        if (m_OpenMode != Mode::Read)
        {
            throw std::invalid_argument(
                "ERROR: BPFileReader only supports OpenMode::Read from" +
                m_Name + " " + m_EndMessage);
        }
    }

    InitTransports();
    InitBuffer();
}

void BP4Reader::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
    // TODO Set Parameters

    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        const std::string metadataFile(
            m_BP4Deserializer.GetBPMetadataFileName(m_Name));

        const bool profile = m_BP4Deserializer.m_Profiler.IsActive;
        m_FileManager.OpenFiles({metadataFile}, adios2::Mode::Read,
                                m_IO.m_TransportsParameters, profile);

        /* Open file to save the metadata index table */
        const std::string metadataIndexFile(
            m_BP4Deserializer.GetBPMetadataIndexFileName(m_Name));
        m_FileMetadataIndexManager.OpenFiles(
            {metadataIndexFile}, adios2::Mode::Read,
            m_IO.m_TransportsParameters, profile);
    }
}

void BP4Reader::InitBuffer()
{
    // Put all metadata in buffer
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        const size_t fileSize = m_FileManager.GetFileSize(0);
        m_BP4Deserializer.m_Metadata.Resize(
            fileSize, "allocating metadata buffer, in call to BP4Reader Open");

        m_FileManager.ReadFile(m_BP4Deserializer.m_Metadata.m_Buffer.data(),
                               fileSize);

        const size_t metadataIndexFileSize =
            m_FileMetadataIndexManager.GetFileSize(0);
        m_BP4Deserializer.m_MetadataIndex.Resize(
            metadataIndexFileSize,
            "allocating metadata index buffer, in call to BPFileReader Open");
        m_FileMetadataIndexManager.ReadFile(
            m_BP4Deserializer.m_MetadataIndex.m_Buffer.data(),
            metadataIndexFileSize);
    }
    // broadcast buffer to all ranks from zero
    helper::BroadcastVector(m_BP4Deserializer.m_Metadata.m_Buffer, m_MPIComm);

    // broadcast metadata index buffer to all ranks from zero
    helper::BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer,
                            m_MPIComm);

    /* Parse metadata index table */
    m_BP4Deserializer.ParseMetadataIndex(m_BP4Deserializer.m_MetadataIndex);

    // fills IO with Variables and Attributes
    m_BP4Deserializer.ParseMetadata(m_BP4Deserializer.m_Metadata, *this);
}

#define declare_type(T)                                                        \
    void BP4Reader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::Get");                                    \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP4Reader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::Get");                                    \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP4Reader::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Reader::Close");
    PerformGets();
    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    BP4Reader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::AllStepsBlocksInfo");                     \
        return m_BP4Deserializer.AllStepsBlocksInfo(variable);                 \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> BP4Reader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::BlocksInfo");                             \
        return m_BP4Deserializer.BlocksInfo(variable, step);                   \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

size_t BP4Reader::DoSteps() const
{
    return m_BP4Deserializer.m_MetadataSet.StepsCount;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
