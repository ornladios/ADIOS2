/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileWriter.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPFileWriter.h"
#include "BPFileWriter.tcc"

#include <iostream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/transport/file/FileFStream.h"

namespace adios2
{

BPFileWriter::BPFileWriter(IO &io, const std::string &name, const Mode mode,
                           MPI_Comm mpiComm)
: Engine("BPFileWriter", io, name, mode, mpiComm),
  m_BP3Serializer(mpiComm, m_DebugMode),
  m_FileDataManager(mpiComm, m_DebugMode),
  m_FileMetadataManager(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open BPFileWriter " + m_Name + "\n";
    Init();
}

BPFileWriter::~BPFileWriter() = default;

StepStatus BPFileWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_BP3Serializer.m_DeferredVariables.clear();
    m_BP3Serializer.m_DeferredVariablesDataSize = 0;
    return StepStatus::OK;
}

void BPFileWriter::PerformPuts()
{
    m_BP3Serializer.ResizeBuffer(m_BP3Serializer.m_DeferredVariablesDataSize,
                                 "in call to PerformPuts");

    for (const auto &variableName : m_BP3Serializer.m_DeferredVariables)
    {
        PutSync(variableName);
    }

    m_BP3Serializer.m_DeferredVariables.clear();
}

void BPFileWriter::EndStep()
{
    if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    m_BP3Serializer.SerializeData(m_IO, true); // true: advances step

    const size_t currentStep = m_BP3Serializer.m_MetadataSet.TimeStep - 1;
    const size_t flushStepsCount = m_BP3Serializer.m_FlushStepsCount;

    if (currentStep % flushStepsCount)
    {
        m_BP3Serializer.SerializeData(m_IO);
        m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(),
                                     m_BP3Serializer.m_Data.m_Position);
        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Data);
        WriteCollectiveMetadataFile();
    }
}

// PRIVATE FUNCTIONS
// PRIVATE
void BPFileWriter::Init()
{
    InitParameters();
    InitTransports();
    InitBPBuffer();
}

#define declare_type(T)                                                        \
    void BPFileWriter::DoPutSync(Variable<T> &variable, const T *values)       \
    {                                                                          \
        PutSyncCommon(variable, values);                                       \
    }                                                                          \
    void BPFileWriter::DoPutDeferred(Variable<T> &variable, const T *values)   \
    {                                                                          \
        PutDeferredCommon(variable, values);                                   \
    }                                                                          \
    void BPFileWriter::DoPutDeferred(Variable<T> &, const T &value) {}
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

void BPFileWriter::InitParameters()
{
    m_BP3Serializer.InitParameters(m_IO.m_Parameters);
}

void BPFileWriter::InitTransports()
{
    // TODO need to add support for aggregators here later
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    // Names passed to IO AddTransport option with key "Name"
    const std::vector<std::string> transportsNames =
        m_FileDataManager.GetFilesBaseNames(m_Name,
                                            m_IO.m_TransportsParameters);

    // /path/name.bp.dir/name.bp.rank
    const std::vector<std::string> bpRankNames =
        m_BP3Serializer.GetBPRankNames(transportsNames);

    m_FileDataManager.OpenFiles(bpRankNames, m_OpenMode,
                                m_IO.m_TransportsParameters,
                                m_BP3Serializer.m_Profiler.IsActive);
}

void BPFileWriter::InitBPBuffer()
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
            m_IO.m_HostLanguage, m_FileDataManager.GetTransportsTypes());
    }
}

void BPFileWriter::DoClose(const int transportIndex)
{
    if (m_BP3Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    // close bp buffer by serializing data and metadata
    m_BP3Serializer.CloseData(m_IO);
    // send data to corresponding transports
    m_FileDataManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(),
                                 m_BP3Serializer.m_Data.m_Position,
                                 transportIndex);

    m_FileDataManager.CloseFiles(transportIndex);

    if (m_BP3Serializer.m_Profiler.IsActive &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteProfilingJSONFile();
    }

    if (m_BP3Serializer.m_CollectiveMetadata &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteCollectiveMetadataFile();
    }
}

void BPFileWriter::WriteProfilingJSONFile()
{
    auto transportTypes = m_FileDataManager.GetTransportsTypes();
    auto transportProfilers = m_FileDataManager.GetTransportsProfilers();

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

void BPFileWriter::WriteCollectiveMetadataFile()
{
    m_BP3Serializer.AggregateCollectiveMetadata();
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

        m_BP3Serializer.ResetBuffer(m_BP3Serializer.m_Metadata, true);
    }
}

} // end namespace adios2
