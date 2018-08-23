/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4FileReader.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4FileReader.h"
#include "BP4FileReader.tcc"

#include "adios2/helper/adiosFunctions.h" // MPI BroadcastVector

namespace adios2
{
namespace core
{
namespace engine
{

BP4FileReader::BP4FileReader(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("BP4FileReader", io, name, mode, mpiComm),
  m_BP4Deserializer(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode),
  m_SubFileManager(mpiComm, m_DebugMode), m_FileMetadataIndexManager(mpiComm, m_DebugMode)
{
    Init();
}

StepStatus BP4FileReader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    if (m_DebugMode)
    {
        if (mode != StepMode::NextAvailable)
        {
            throw std::invalid_argument("ERROR: mode is not supported yet, "
                                        "only NextAvailable is valid for "
                                        "engine BP4FileReader, in call to "
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

    if (m_CurrentStep >= m_BP4Deserializer.m_MetadataSet.StepsCount)
    {
        return StepStatus::EndOfStream;
    }

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
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    return StepStatus::OK;
}

size_t BP4FileReader::CurrentStep() const { return m_CurrentStep; }

void BP4FileReader::EndStep() { PerformGets(); }

void BP4FileReader::PerformGets()
{
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
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    m_BP4Deserializer.m_DeferredVariables.clear();
}

// PRIVATE
void BP4FileReader::Init()
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

void BP4FileReader::InitTransports()
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
        
        /*Lipeng*/
        const std::string metadataIndexFile(m_BP4Deserializer.GetBPMetadataIndexFileName(m_Name));
        m_FileMetadataIndexManager.OpenFiles({metadataIndexFile}, adios2::Mode::Read,
                                m_IO.m_TransportsParameters, profile);
    }
}

void BP4FileReader::InitBuffer()
{
    // Put all metadata in buffer
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        const size_t fileSize = m_FileManager.GetFileSize(0);
        m_BP4Deserializer.m_Metadata.Resize(
            fileSize,
            "allocating metadata buffer, in call to BP4FileReader Open");

        m_FileManager.ReadFile(m_BP4Deserializer.m_Metadata.m_Buffer.data(),
                               fileSize);

        const size_t metadataIndexFileSize = m_FileMetadataIndexManager.GetFileSize(0);
        m_BP4Deserializer.m_MetadataIndex.Resize(
            metadataIndexFileSize,
            "allocating metadata index buffer, in call to BPFileReader Open");
        m_FileMetadataIndexManager.ReadFile(m_BP4Deserializer.m_MetadataIndex.m_Buffer.data(),
                               metadataIndexFileSize);
    }
    // broadcast buffer to all ranks from zero
    helper::BroadcastVector(m_BP4Deserializer.m_Metadata.m_Buffer, m_MPIComm);

    // broadcast metadata index buffer to all ranks from zero
    helper::BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer, m_MPIComm);

    /*Lipeng*/
    m_BP4Deserializer.ParseMetadataIndex(m_BP4Deserializer.m_MetadataIndex);

    // fills IO with Variables and Attributes
    m_BP4Deserializer.ParseMetadata(m_BP4Deserializer.m_Metadata, m_IO);
}

#define declare_type(T)                                                        \
    void BP4FileReader::DoGetSync(Variable<T> &variable, T *data)               \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP4FileReader::DoGetDeferred(Variable<T> &variable, T *data)           \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BP4FileReader::DoClose(const int transportIndex)
{
    PerformGets();
    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    BP4FileReader::DoAllStepsBlocksInfo(const Variable<T> &variable) const      \
    {                                                                          \
        return m_BP4Deserializer.AllStepsBlocksInfo(variable);                 \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> BP4FileReader::DoBlocksInfo(        \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return m_BP4Deserializer.BlocksInfo(variable, step);                   \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
