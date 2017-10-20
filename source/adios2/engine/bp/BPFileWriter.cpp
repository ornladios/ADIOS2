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
#include <adios2/toolkit/transport/file/FileFStream.h>

#include <iostream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange

namespace adios2
{

BPFileWriter::BPFileWriter(IO &io, const std::string &name, const Mode openMode,
                           MPI_Comm mpiComm)
: Engine("BPFileWriter", io, name, openMode, mpiComm),
  m_BP3Serializer(mpiComm, m_DebugMode), m_FileManager(mpiComm, m_DebugMode)
{
    m_EndMessage = " in call to IO Open BPFileWriter " + m_Name + "\n";
    Init();
}

BPFileWriter::~BPFileWriter() = default;

void BPFileWriter::BeginStep() {}

void BPFileWriter::EndStep()
{
    m_BP3Serializer.SerializeData(m_IO, true); // true: advances step
}

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

void BPFileWriter::Close(const int transportIndex)
{
    //    if (m_DebugMode)
    //    {
    //        if (!m_FileManager.CheckTransportIndex(transportIndex))
    //       {
    //            auto transportsSize = m_FileManager.m_Transports.size();
    //            throw std::invalid_argument(
    //                "ERROR: transport index " + std::to_string(transportIndex)
    //                +
    //                " outside range, -1 (default) to " +
    //                std::to_string(transportsSize - 1) + ", in call to
    //                Close\n");
    //        }
    //    }

    // close bp buffer by serializing data and metadata
    m_BP3Serializer.CloseData(m_IO);
    // send data to corresponding transports
    m_FileManager.WriteFiles(m_BP3Serializer.m_Data.m_Buffer.data(),
                             m_BP3Serializer.m_Data.m_Position, transportIndex);

    m_FileManager.CloseFiles(transportIndex);

    if (m_BP3Serializer.m_Profiler.IsActive &&
        m_FileManager.AllTransportsClosed())
    {
        WriteProfilingJSONFile();
    }

    if (m_BP3Serializer.m_CollectiveMetadata &&
        m_FileManager.AllTransportsClosed())
    {
        WriteCollectiveMetadataFile();
    }
}

// PRIVATE FUNCTIONS
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

    // Names are std::vector<std::string>
    auto transportsNames =
        m_FileManager.GetFilesBaseNames(m_Name, m_IO.m_TransportsParameters);
    auto bpBaseNames = m_BP3Serializer.GetBPBaseNames(transportsNames);
    auto bpNames = m_BP3Serializer.GetBPNames(transportsNames);

    m_FileManager.OpenFiles(bpBaseNames, bpNames, m_OpenMode,
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
            m_IO.m_HostLanguage, m_FileManager.GetTransportsTypes());
    }
}

void BPFileWriter::WriteProfilingJSONFile()
{
    auto transportTypes = m_FileManager.GetTransportsTypes();
    auto transportProfilers = m_FileManager.GetTransportsProfilers();

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
        transport::FileFStream metadataStream(m_MPIComm, m_DebugMode);
        const std::string metadataFileName(
            m_BP3Serializer.GetBPMetadataFileName(m_Name));
        metadataStream.Open(metadataFileName, Mode::Write);
        metadataStream.Write(m_BP3Serializer.m_Metadata.m_Buffer.data(),
                             m_BP3Serializer.m_Metadata.m_AbsolutePosition);
        metadataStream.Close();
    }
}

} // end namespace adios2
