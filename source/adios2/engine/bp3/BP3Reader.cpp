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

#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

namespace adios2
{
namespace core
{
namespace engine
{

BP3Reader::BP3Reader(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("BP3", io, name, mode, std::move(comm)),
  m_BP3Deserializer(m_Comm, m_DebugMode), m_FileManager(m_Comm, m_DebugMode),
  m_SubFileManager(m_Comm, m_DebugMode)
{
    TAU_SCOPED_TIMER("BP3Reader::Open");
    Init();
}

StepStatus BP3Reader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER("BP3Reader::BeginStep");
    if (m_DebugMode)
    {
        if (mode != StepMode::Read)
        {
            throw std::invalid_argument(
                "ERROR: mode is not supported yet, "
                "only Read is valid for "
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

    if (m_CurrentStep >= m_BP3Deserializer.m_MetadataSet.StepsCount)
    {
        m_IO.m_ReadStreaming = false;
        return StepStatus::EndOfStream;
    }

    m_IO.ResetVariablesStepSelection(false, "in call to BP3 Reader BeginStep");

    return StepStatus::OK;
}

size_t BP3Reader::CurrentStep() const { return m_CurrentStep; }

void BP3Reader::EndStep()
{
    TAU_SCOPED_TIMER("BP3Reader::EndStep");
    PerformGets();
}

void BP3Reader::PerformGets()
{
    TAU_SCOPED_TIMER("BP3Reader::PerformGets");
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
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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
        const bool profile = m_BP3Deserializer.m_Profiler.m_IsActive;
        try
        {
            m_FileManager.OpenFiles({m_Name}, adios2::Mode::Read,
                                    m_IO.m_TransportsParameters, profile);
        }
        catch (...)
        {
            const std::string bpName = helper::AddExtension(m_Name, ".bp");
            m_FileManager.OpenFiles({bpName}, adios2::Mode::Read,
                                    m_IO.m_TransportsParameters, profile);
        }
    }
}

void BP3Reader::InitBuffer()
{
    if (m_BP3Deserializer.m_RankMPI == 0)
    {
        const size_t fileSize = m_FileManager.GetFileSize();
        // handle single bp files from ADIOS 1.x by getting onl the metadata in
        // buffer

        // Load/Read Minifooter
        const size_t miniFooterSize =
            m_BP3Deserializer.m_MetadataSet.MiniFooterSize;
        const size_t miniFooterStart =
            helper::GetDistance(fileSize, miniFooterSize, m_DebugMode,
                                " fileSize < miniFooterSize, in call to Open");

        m_BP3Deserializer.m_Metadata.Resize(
            miniFooterSize,
            "allocating metadata buffer to inspect bp minifooter, in call to "
            "Open");

        m_FileManager.ReadFile(m_BP3Deserializer.m_Metadata.m_Buffer.data(),
                               miniFooterSize, miniFooterStart);

        // Load/Read Metadata
        const size_t metadataStart =
            m_BP3Deserializer.MetadataStart(m_BP3Deserializer.m_Metadata);
        const size_t metadataSize =
            helper::GetDistance(fileSize, metadataStart, m_DebugMode,
                                " fileSize < miniFooterSize, in call to Open");

        m_BP3Deserializer.m_Metadata.Resize(
            metadataSize, "allocating metadata buffer, in call to Open");

        m_FileManager.ReadFile(m_BP3Deserializer.m_Metadata.m_Buffer.data(),
                               metadataSize, metadataStart);
    }

    // broadcast metadata buffer to all ranks from zero
    m_Comm.BroadcastVector(m_BP3Deserializer.m_Metadata.m_Buffer);

    // fills IO with available Variables and Attributes
    m_BP3Deserializer.ParseMetadata(m_BP3Deserializer.m_Metadata, *this);
    // caches attributes associated with variables
    m_IO.SetPrefixedNames(false);
}

#define declare_type(T)                                                        \
    void BP3Reader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        TAU_SCOPED_TIMER("BP3Reader::Get");                                    \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP3Reader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        TAU_SCOPED_TIMER("BP3Reader::Get");                                    \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP3Reader::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("BP3Reader::Close");
    PerformGets();
    m_SubFileManager.CloseFiles();
    m_FileManager.CloseFiles();
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    BP3Reader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        TAU_SCOPED_TIMER("BP3Reader::AllStepsBlocksInfo");                     \
        return m_BP3Deserializer.AllStepsBlocksInfo(variable);                 \
    }                                                                          \
                                                                               \
    std::vector<std::vector<typename Variable<T>::Info>>                       \
    BP3Reader::DoAllRelativeStepsBlocksInfo(const Variable<T> &variable) const \
    {                                                                          \
        TAU_SCOPED_TIMER("BP3Reader::AllRelativeStepsBlocksInfo");             \
        return m_BP3Deserializer.AllRelativeStepsBlocksInfo(variable);         \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> BP3Reader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        TAU_SCOPED_TIMER("BP3Reader::BlocksInfo");                             \
        return m_BP3Deserializer.BlocksInfo(variable, step);                   \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

size_t BP3Reader::DoSteps() const
{
    return m_BP3Deserializer.m_MetadataSet.StepsCount;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
