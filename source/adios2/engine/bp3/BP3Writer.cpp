/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Writer.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP3Writer.h"
#include "BP3Writer.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/transport/file/FileFStream.h"

namespace adios2
{
namespace core
{
namespace engine
{

BP3Writer::BP3Writer(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("BP3", io, name, mode, mpiComm), m_BP3Serializer(mpiComm, m_DebugMode),
  m_FileDataManager(mpiComm, m_DebugMode),
  m_FileMetadataManager(mpiComm, m_DebugMode)
{
    m_IO.m_ReadStreaming = false;
    m_EndMessage = " in call to IO Open BPFileWriter " + m_Name + "\n";
    Init();
}

BP3Writer::~BP3Writer() = default;

StepStatus BP3Writer::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_BP3Serializer.m_DeferredVariables.clear();
    m_BP3Serializer.m_DeferredVariablesDataSize = 0;
    m_IO.m_ReadStreaming = false;
    return StepStatus::OK;
}

size_t BP3Writer::CurrentStep() const
{
    return m_BP3Serializer.m_MetadataSet.CurrentStep;
}

void BP3Writer::PerformPuts()
{
    if (m_BP3Serializer.m_DeferredVariables.empty())
    {
        return;
    }

    m_BP3Serializer.ResizeBuffer(m_BP3Serializer.m_DeferredVariablesDataSize,
                                 "in call to PerformPuts");

    for (const std::string &variableName : m_BP3Serializer.m_DeferredVariables)
    {
        const DataType type = m_IO.InquireVariableType(variableName);
        if (type == DataType::Compound)
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        Variable<T> &variable = FindVariable<T>(                               \
            variableName, "in call to PerformPuts, EndStep or Close");         \
                                                                               \
        for (const auto &blockInfo : variable.m_BlocksInfo)                    \
        {                                                                      \
            PutSyncCommon(variable, blockInfo);                                \
        }                                                                      \
        variable.m_BlocksInfo.clear();                                         \
    }

        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
    m_BP3Serializer.m_DeferredVariables.clear();
}

void BP3Writer::EndStep()
{
    if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    // true: advances step
    m_BP3Serializer.SerializeData(m_IO, true);

    const size_t currentStep = CurrentStep();
    const size_t flushStepsCount = m_BP3Serializer.m_FlushStepsCount;

    if (currentStep % flushStepsCount == 0)
    {
        Flush();
    }
}

void BP3Writer::Flush(const int transportIndex)
{
    DoFlush(false, transportIndex);
    m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);

    if (m_BP3Serializer.m_CollectiveMetadata)
    {
        WriteCollectiveMetadataFile();
    }
}

// PRIVATE
void BP3Writer::Init()
{
    InitParameters();
    InitTransports();
    InitBPBuffer();
}

#define declare_type(T)                                                        \
    void BP3Writer::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, variable.SetBlockInfo(data, CurrentStep()));   \
        variable.m_BlocksInfo.clear();                                         \
    }                                                                          \
    void BP3Writer::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP3Writer::InitParameters()
{
    m_BP3Serializer.InitParameters(m_IO.m_Parameters);
}

void BP3Writer::InitTransports()
{
    // TODO need to add support for aggregators here later
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    // only consumers will interact with transport managers
    std::vector<std::string> bpSubStreamNames;

    if (m_BP3Serializer.m_Aggregator.m_IsConsumer)
    {
        // Names passed to IO AddTransport option with key "Name"
        const std::vector<std::string> transportsNames =
            m_FileDataManager.GetFilesBaseNames(m_Name,
                                                m_IO.m_TransportsParameters);

        // /path/name.bp.dir/name.bp.rank
        bpSubStreamNames = m_BP3Serializer.GetBPSubStreamNames(transportsNames);
    }

    m_BP3Serializer.ProfilerStart("mkdir");
    m_FileDataManager.MkDirsBarrier(bpSubStreamNames,
                                    m_BP3Serializer.m_NodeLocal);
    m_BP3Serializer.ProfilerStop("mkdir");

    if (m_BP3Serializer.m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.OpenFiles(bpSubStreamNames, m_OpenMode,
                                    m_IO.m_TransportsParameters,
                                    m_BP3Serializer.m_Profiler.IsActive);
    }
}

void BP3Writer::InitBPBuffer()
{
    if (m_OpenMode == Mode::Append)
    {
        throw std::invalid_argument(
            "ADIOS2: OpenMode Append hasn't been implemented, yet");
        // TODO: Get last pg timestep and update timestep counter in
    }
    else
    {
        m_BP3Serializer.PutProcessGroupIndex(
            m_IO.m_Name, m_IO.m_HostLanguage,
            m_FileDataManager.GetTransportsTypes());
    }
}

void BP3Writer::DoFlush(const bool isFinal, const int transportIndex)
{
    if (m_BP3Serializer.m_Aggregator.m_IsActive)
    {
        AggregateWriteData(isFinal, transportIndex);
    }
    else
    {
        WriteData(isFinal, transportIndex);
    }
}

void BP3Writer::DoClose(const int transportIndex)
{
    if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    DoFlush(true, transportIndex);

    if (m_BP3Serializer.m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.CloseFiles(transportIndex);
    }

    if (m_BP3Serializer.m_CollectiveMetadata &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteCollectiveMetadataFile(true);
    }

    if (m_BP3Serializer.m_Profiler.IsActive &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteProfilingJSONFile();
    }
}

void BP3Writer::WriteProfilingJSONFile()
{
    auto transportTypes = m_FileDataManager.GetTransportsTypes();
    auto transportProfilers = m_FileDataManager.GetTransportsProfilers();

    auto transportTypesMD = m_FileMetadataManager.GetTransportsTypes();
    auto transportProfilersMD = m_FileMetadataManager.GetTransportsProfilers();

    transportTypes.insert(transportTypes.end(), transportTypesMD.begin(),
                          transportTypesMD.end());

    transportProfilers.insert(transportProfilers.end(),
                              transportProfilersMD.begin(),
                              transportProfilersMD.end());

    const std::string lineJSON(m_BP3Serializer.GetRankProfilingJSON(
                                   transportTypes, transportProfilers) +
                               ",\n");

    const std::vector<char> profilingJSON(
        m_BP3Serializer.AggregateProfilingJSON(lineJSON));

    if (m_BP3Serializer.m_RankMPI == 0)
    {
        transport::FileFStream profilingJSONStream(m_MPIComm, m_DebugMode);
        auto bpBaseNames = m_BP3Serializer.GetBPBaseNames({m_Name});
        profilingJSONStream.Open(bpBaseNames[0] + "/profiling.json",
                                 Mode::Write);
        profilingJSONStream.Write(profilingJSON.data(), profilingJSON.size());
        profilingJSONStream.Close();
    }
}

void BP3Writer::WriteCollectiveMetadataFile(const bool isFinal)
{
    m_BP3Serializer.AggregateCollectiveMetadata(
        m_MPIComm, m_BP3Serializer.m_Metadata, true);

    if (m_BP3Serializer.m_RankMPI == 0)
    {
        // first init metadata files
        const std::vector<std::string> transportsNames =
            m_FileMetadataManager.GetFilesBaseNames(
                m_Name, m_IO.m_TransportsParameters);

        const std::vector<std::string> bpMetadataFileNames =
            m_BP3Serializer.GetBPMetadataFileNames(transportsNames);

        m_FileMetadataManager.OpenFiles(bpMetadataFileNames, m_OpenMode,
                                        m_IO.m_TransportsParameters,
                                        m_BP3Serializer.m_Profiler.IsActive);

        m_FileMetadataManager.WriteFiles(
            m_BP3Serializer.m_Metadata.m_Buffer.data(),
            m_BP3Serializer.m_Metadata.m_Position);
        m_FileMetadataManager.CloseFiles();

        if (!isFinal)
        {
            m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Metadata, true);
            m_FileMetadataManager.m_Transports.clear();
        }
    }
}

void BP3Writer::WriteData(const bool isFinal, const int transportIndex)
{
    size_t dataSize = m_BP3Serializer.m_Data.m_Position;

    if (isFinal)
    {
        m_BP3Serializer.CloseData(m_IO);
        dataSize = m_BP3Serializer.m_Data.m_Position;
    }
    else
    {
        m_BP3Serializer.CloseStream(m_IO);
    }

    m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(),
                                 dataSize, transportIndex);

    m_FileDataManager.FlushFiles(transportIndex);
}

void BP3Writer::AggregateWriteData(const bool isFinal, const int transportIndex)
{
    m_BP3Serializer.CloseStream(m_IO, false);

    // async?
    for (int r = 0; r < m_BP3Serializer.m_Aggregator.m_Size; ++r)
    {
        std::vector<MPI_Request> dataRequests =
            m_BP3Serializer.m_Aggregator.IExchange(m_BP3Serializer.m_Data, r);

        std::vector<MPI_Request> absolutePositionRequests =
            m_BP3Serializer.m_Aggregator.IExchangeAbsolutePosition(
                m_BP3Serializer.m_Data, r);

        if (m_BP3Serializer.m_Aggregator.m_IsConsumer)
        {
            const BufferSTL &bufferSTL =
                m_BP3Serializer.m_Aggregator.GetConsumerBuffer(
                    m_BP3Serializer.m_Data);

            m_FileDataManager.WriteFiles(bufferSTL.m_Buffer.data(),
                                         bufferSTL.m_Position, transportIndex);

            m_FileDataManager.FlushFiles(transportIndex);
        }

        m_BP3Serializer.m_Aggregator.WaitAbsolutePosition(
            absolutePositionRequests, r);

        m_BP3Serializer.m_Aggregator.Wait(dataRequests, r);
        m_BP3Serializer.m_Aggregator.SwapBuffers(r);
    }

    m_BP3Serializer.UpdateOffsetsInMetadata();

    if (isFinal) // Write metadata footer
    {
        BufferSTL &bufferSTL = m_BP3Serializer.m_Data;
        m_BP3Serializer.ResetBuffer(bufferSTL, false, false);

        m_BP3Serializer.AggregateCollectiveMetadata(
            m_BP3Serializer.m_Aggregator.m_Comm, bufferSTL, false);

        if (m_BP3Serializer.m_Aggregator.m_IsConsumer)
        {
            m_FileDataManager.WriteFiles(bufferSTL.m_Buffer.data(),
                                         bufferSTL.m_Position, transportIndex);

            m_FileDataManager.FlushFiles(transportIndex);
        }

        m_BP3Serializer.m_Aggregator.Close();
    }

    m_BP3Serializer.m_Aggregator.ResetBuffers();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
