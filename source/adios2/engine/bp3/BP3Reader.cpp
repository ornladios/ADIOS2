/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Reader.cpp
 *
 *  Created on: Feb 27, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP3Reader.h"
#include "BP3Reader.tcc"

#include "adios2/helper/adiosFunctions.h" // MPI BroadcastVector

namespace adios2
{
namespace core
{
namespace engine
{

BP3Reader::BP3Reader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("BPFileReader", io, name, mode, mpiComm),
  m_BP3Deserializer(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode),
  m_SubFileManager(mpiComm, m_DebugMode)
{
    Init();
}

StepStatus BP3Reader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    if (m_DebugMode)
    {
        if (mode != StepMode::NextAvailable)
        {
            throw std::invalid_argument(
                "ERROR: mode is not supported yet, "
                "only NextAvailable is valid for "
                "engine BP3 with adios2::Mode::Read, in call to "
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

    bool localFirstStep = false;
    if (m_FirstStep)
    {
        localFirstStep = true;
        m_FirstStep = false;
    }
    else
    {
        ++m_CurrentStep;
    }

    // used to inquire for variables in streaming mode
    m_IO.m_Streaming = true;
    m_IO.m_EngineStep = m_CurrentStep;

    if (m_CurrentStep >= m_BP3Deserializer.m_MetadataSet.StepsCount)
    {
        m_IO.m_Streaming = false;
        return StepStatus::EndOfStream;
    }

    m_IO.ResetVariablesStepSelection(false, "in call to BP3 Reader BeginStep");

    return StepStatus::OK;
}

size_t BP3Reader::CurrentStep() const { return m_CurrentStep; }

void BP3Reader::EndStep() { PerformGets(); }

void BP3Reader::PerformGets()
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
void BP3Reader::Init()
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

void BP3Reader::InitTransports()
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

void BP3Reader::InitBuffer()
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
    void BP3Reader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP3Reader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BP3Reader::DoClose(const int transportIndex)
{
    PerformGets();
    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    BP3Reader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        return m_BP3Deserializer.AllStepsBlocksInfo(variable);                 \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> BP3Reader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return m_BP3Deserializer.BlocksInfo(variable, step);                   \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
