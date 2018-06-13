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
namespace core
{
namespace engine
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

        if (!m_BP3Deserializer.m_DeferredVariables.empty())
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

size_t BPFileReader::CurrentStep() const { return m_CurrentStep; }

void BPFileReader::EndStep() { PerformGets(); }

void BPFileReader::PerformGets()
{
    if (m_BP3Deserializer.m_DeferredVariables.empty())
    {
        return;
    }

    for (const std::string &name : m_BP3Deserializer.m_DeferredVariables)
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
            m_BP3Deserializer.SetVariableBlockInfo(variable, blockInfo);       \
        }                                                                      \
        ReadVariableBlocks(variable);                                          \
        variable.m_BlocksInfo.clear();                                         \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    m_BP3Deserializer.m_DeferredVariables.clear();
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
    helper::BroadcastVector(m_BP3Deserializer.m_Metadata.m_Buffer, m_MPIComm);

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
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BPFileReader::DoClose(const int transportIndex)
{
    PerformGets();
    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
