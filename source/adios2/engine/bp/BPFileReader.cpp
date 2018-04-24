/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileReader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPFileReader.h"
#include "BPFileReader.tcc"

#include "adios2/helper/adiosFunctions.h" // MPI BroadcastVector

namespace adios2
{

BPFileReader::BPFileReader(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("BPFileReader", io, name, mode, mpiComm),
  m_BP3Deserializer(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode),
  m_SubFileManager(mpiComm, m_DebugMode)
{
    Init();
}

StepStatus BPFileReader::BeginStep(StepMode mode, const float timeoutSeconds)
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

        if (!m_BP3Deserializer.m_PerformedGets)
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

    if (m_CurrentStep >= m_BP3Deserializer.m_MetadataSet.StepsCount)
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

size_t BPFileReader::CurrentStep() const { return m_CurrentStep; }

void BPFileReader::EndStep()
{
    if (!m_BP3Deserializer.m_PerformedGets)
    {
        PerformGets();
    }
}

void BPFileReader::PerformGets()
{
    const std::map<std::string, SubFileInfoMap> variablesSubfileInfo =
        m_BP3Deserializer.PerformGetsVariablesSubFileInfo(m_IO);
    ReadVariables(variablesSubfileInfo);
    m_BP3Deserializer.m_PerformedGets = true;
}

// PRIVATE
void BPFileReader::Init()
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

void BPFileReader::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
    // TODO Set Parameters

    if (m_BP3Deserializer.m_RankMPI == 0)
    {
        const std::string metadataFile(
            m_BP3Deserializer.GetBPMetadataFileName(m_Name));

        const bool profile = m_BP3Deserializer.m_Profiler.IsActive;
        m_FileManager.OpenFiles({metadataFile}, adios2::Mode::Read,
                                m_IO.m_TransportsParameters, profile);
    }
}

void BPFileReader::InitBuffer()
{
    // Put all metadata in buffer
    if (m_BP3Deserializer.m_RankMPI == 0)
    {
        const size_t fileSize = m_FileManager.GetFileSize(0);
        m_BP3Deserializer.m_Metadata.Resize(
            fileSize,
            "allocating metadata buffer, in call to BPFileReader Open");

        m_FileManager.ReadFile(m_BP3Deserializer.m_Metadata.m_Buffer.data(),
                               fileSize);
    }
    // broadcast buffer to all ranks from zero
    BroadcastVector(m_BP3Deserializer.m_Metadata.m_Buffer, m_MPIComm);

    // fills IO with Variables and Attributes
    m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Metadata, m_IO);
}

#define declare_type(T)                                                        \
    void BPFileReader::DoGetSync(Variable<T> &variable, T *data)               \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BPFileReader::DoGetDeferred(Variable<T> &variable, T *data)           \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }                                                                          \
    void BPFileReader::DoGetDeferred(Variable<T> &variable, T &data)           \
    {                                                                          \
        GetDeferredCommon(variable, &data);                                    \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BPFileReader::ReadVariables(
    const std::map<std::string, SubFileInfoMap> &variablesSubFileInfo)
{
    const bool profile = m_BP3Deserializer.m_Profiler.IsActive;

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
                    m_BP3Deserializer.GetBPSubFileName(m_Name, subFileIndex));

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

                    m_BP3Deserializer.ClipContiguousMemory(
                        variableName, m_IO, contiguousMemory,
                        blockInfo.BlockBox, blockInfo.IntersectionBox);
                } // end block

                // Advancing data pointer for the next step
                m_BP3Deserializer.SetVariableNextStepData(variableName, m_IO);

            } // end step
        }     // end subfile
    }         // end variable
}

void BPFileReader::DoClose(const int transportIndex)
{
    if (!m_BP3Deserializer.m_PerformedGets)
    {
        PerformGets();
    }

    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
}

} // end namespace adios2
