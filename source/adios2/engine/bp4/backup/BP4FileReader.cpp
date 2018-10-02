/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4FileReader.cpp
 *
 *  Created on: Jul 31, 2018
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
                                        "engine BPFileReader, in call to "
                                        "BeginStep\n");
        }

        if (!m_BP4Deserializer.m_PerformedGets)
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
    else if (type == GetType<T>())                                             \
    {                                                                          \
        auto variable = m_IO.InquireVariable<T>(name);                         \
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

void BP4FileReader::EndStep()
{
    if (!m_BP4Deserializer.m_PerformedGets)
    {
        PerformGets();
    }
}

void BP4FileReader::PerformGets()
{
    const std::map<std::string, helper::SubFileInfoMap> variablesSubfileInfo =
        m_BP4Deserializer.PerformGetsVariablesSubFileInfo(m_IO);
    ReadVariables(variablesSubfileInfo);
    m_BP4Deserializer.m_PerformedGets = true;
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

        const std::string metadataIndexFile(metadataFile+".metadata.index");
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
            "allocating metadata buffer, in call to BPFileReader Open");

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

    helper::BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer, m_MPIComm);

    m_BP4Deserializer.ParseMetadataIndex(m_BP4Deserializer.m_MetadataIndex);

    m_BP4Deserializer.ParseMetadata(m_BP4Deserializer.m_Metadata, m_IO);

    // fills IO with Variables and Attributes
    //m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Metadata, m_IO);
}

#define declare_type(T)                                                        \
    void BP4FileReader::DoGetSync(Variable<T> &variable, T *data)               \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP4FileReader::DoGetDeferred(Variable<T> &variable, T *data)           \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BP4FileReader::ReadVariables(
    const std::map<std::string, helper::SubFileInfoMap> &variablesSubFileInfo)
{
    const bool profile = m_BP4Deserializer.m_Profiler.IsActive;

    // sequentially request bytes from transport manager
    // threaded per variable?
    for (const auto &variableNamePair : variablesSubFileInfo) // variable name
    {
        const std::string variableName(variableNamePair.first);

        // or threaded per file?
        for (const auto &subFileIndexPair : variableNamePair.second)
        {
            const size_t subFileIndex = subFileIndexPair.first;

            if (m_SubFileManager.m_Transports.count(subFileIndex) == 0)
            {
                const std::string subFile(
                    m_BP4Deserializer.GetBPSubFileName(m_Name, subFileIndex));

                m_SubFileManager.OpenFileID(subFile, subFileIndex, Mode::Read,
                                            {{"transport", "File"}}, profile);
            }

            for (const auto &stepPair : subFileIndexPair.second) // step
            {
                for (const auto &blockInfo : stepPair.second)
                {
                    const auto &seek = blockInfo.Seeks;
                    const size_t blockStart = seek.first;
                    const size_t blockSize = seek.second - seek.first;
                    std::vector<char> contiguousMemory(blockSize);
                    m_SubFileManager.ReadFile(contiguousMemory.data(),
                                              blockSize, blockStart,
                                              subFileIndex);

                    m_BP4Deserializer.ClipContiguousMemory(
                        variableName, m_IO, contiguousMemory,
                        blockInfo.BlockBox, blockInfo.IntersectionBox);
                } // end block

                // Advancing data pointer for the next step
                // m_BP3Deserializer.SetVariableNextStepData(variableName,
                // m_IO);
            } // end step
        }     // end subfile
    }         // end variable
}

void BP4FileReader::DoClose(const int transportIndex)
{
    if (!m_BP4Deserializer.m_PerformedGets)
    {
        PerformGets();
    }

    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
    m_FileMetadataIndexManager.CloseFiles();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
