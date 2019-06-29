/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Writer.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Writer.h"
#include "BP4Writer.tcc"

#include "adios2/common/ADIOSMPI.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <ctime>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

BP4Writer::BP4Writer(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("BP4Writer", io, name, mode, mpiComm),
  m_BP4Serializer(mpiComm, m_DebugMode),
  m_FileDataManager(mpiComm, m_DebugMode),
  m_FileMetadataManager(mpiComm, m_DebugMode),
  m_FileMetadataIndexManager(mpiComm, m_DebugMode)
{
    TAU_SCOPED_TIMER("BP4Writer::Open");
    m_IO.m_ReadStreaming = false;
    m_EndMessage = " in call to IO Open BP4Writer " + m_Name + "\n";
    Init();
}

BP4Writer::~BP4Writer() = default;

StepStatus BP4Writer::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER("BP4Writer::BeginStep");
    m_BP4Serializer.m_DeferredVariables.clear();
    m_BP4Serializer.m_DeferredVariablesDataSize = 0;
    m_IO.m_ReadStreaming = false;
    return StepStatus::OK;
}

size_t BP4Writer::CurrentStep() const
{
    return m_BP4Serializer.m_MetadataSet.CurrentStep;
}

void BP4Writer::PerformPuts()
{
    TAU_SCOPED_TIMER("BP4Writer::PerformPuts");
    if (m_BP4Serializer.m_DeferredVariables.empty())
    {
        return;
    }

    m_BP4Serializer.ResizeBuffer(m_BP4Serializer.m_DeferredVariablesDataSize,
                                 "in call to PerformPuts");

    for (const std::string &variableName : m_BP4Serializer.m_DeferredVariables)
    {
        const std::string type = m_IO.InquireVariableType(variableName);
        if (type == "compound")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
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
    m_BP4Serializer.m_DeferredVariables.clear();
}

void BP4Writer::EndStep()
{
    TAU_SCOPED_TIMER("BP4Writer::EndStep");
    if (m_BP4Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();
    }

    // true: advances step
    m_BP4Serializer.SerializeData(m_IO, true);

    const size_t currentStep = CurrentStep();
    const size_t flushStepsCount = m_BP4Serializer.m_FlushStepsCount;

    if (currentStep % flushStepsCount == 0)
    {
        Flush();
    }
}

void BP4Writer::Flush(const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Writer::Flush");
    DoFlush(false, transportIndex);
    m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Data);

    if (m_BP4Serializer.m_CollectiveMetadata)
    {
        WriteCollectiveMetadataFile();
    }
}

// PRIVATE
void BP4Writer::Init()
{
    InitParameters();
    InitTransports();
    InitBPBuffer();
}

#define declare_type(T)                                                        \
    void BP4Writer::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, variable.SetBlockInfo(data, CurrentStep()));   \
        variable.m_BlocksInfo.clear();                                         \
    }                                                                          \
    void BP4Writer::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP4Writer::InitParameters()
{
    m_BP4Serializer.InitParameters(m_IO.m_Parameters);
}

void BP4Writer::InitTransports()
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

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        // Names passed to IO AddTransport option with key "Name"
        const std::vector<std::string> transportsNames =
            m_FileDataManager.GetFilesBaseNames(m_Name,
                                                m_IO.m_TransportsParameters);

        // /path/name.bp.dir/name.bp.rank
        bpSubStreamNames = m_BP4Serializer.GetBPSubStreamNames(transportsNames);
    }

    m_BP4Serializer.ProfilerStart("mkdir");
    m_FileDataManager.MkDirsBarrier(bpSubStreamNames,
                                    m_BP4Serializer.m_NodeLocal);
    m_BP4Serializer.ProfilerStop("mkdir");

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        // std::cout << "rank " << m_BP4Serializer.m_RankMPI << ": " <<
        // bpSubStreamNames[0] << std::endl;
        m_FileDataManager.OpenFiles(bpSubStreamNames, m_OpenMode,
                                    m_IO.m_TransportsParameters,
                                    m_BP4Serializer.m_Profiler.IsActive);
    }

    if (m_BP4Serializer.m_RankMPI == 0)
    {
        const std::vector<std::string> transportsNames =
            m_FileMetadataManager.GetFilesBaseNames(
                m_Name, m_IO.m_TransportsParameters);

        const std::vector<std::string> bpMetadataFileNames =
            m_BP4Serializer.GetBPMetadataFileNames(transportsNames);

        m_FileMetadataManager.OpenFiles(bpMetadataFileNames, m_OpenMode,
                                        m_IO.m_TransportsParameters,
                                        m_BP4Serializer.m_Profiler.IsActive);

        std::vector<std::string> metadataIndexFileNames =
            m_BP4Serializer.GetBPMetadataIndexFileNames(transportsNames);

        m_FileMetadataIndexManager.OpenFiles(
            metadataIndexFileNames, m_OpenMode, m_IO.m_TransportsParameters,
            m_BP4Serializer.m_Profiler.IsActive);
    }
}

void BP4Writer::InitBPBuffer()
{
    if (m_OpenMode == Mode::Append)
    {
        // throw std::invalid_argument(
        //    "ADIOS2: OpenMode Append hasn't been implemented, yet");
        // TODO: Get last pg timestep and update timestep counter in
        BufferSTL preMetadataIndex;
        size_t preMetadataIndexFileSize;

        if (m_BP4Serializer.m_RankMPI == 0)
        {
            preMetadataIndexFileSize =
                m_FileMetadataIndexManager.GetFileSize(0);
            preMetadataIndex.m_Buffer.resize(preMetadataIndexFileSize);
            preMetadataIndex.m_Buffer.assign(preMetadataIndex.m_Buffer.size(),
                                             '\0');
            preMetadataIndex.m_Position = 0;
            m_FileMetadataIndexManager.ReadFile(
                preMetadataIndex.m_Buffer.data(), preMetadataIndexFileSize);
        }
        helper::BroadcastVector(preMetadataIndex.m_Buffer, m_MPIComm);
        preMetadataIndexFileSize = preMetadataIndex.m_Buffer.size();
        if (preMetadataIndexFileSize > 0)
        {
            size_t position = 0;
            position += 28;
            const uint8_t endianness =
                helper::ReadValue<uint8_t>(preMetadataIndex.m_Buffer, position);
            bool IsLittleEndian = true;
            IsLittleEndian = (endianness == 0) ? true : false;
            if (helper::IsLittleEndian() != IsLittleEndian)
            {
                throw std::runtime_error(
                    "ERROR: previous run generated BigEndian bp file, "
                    "this version of ADIOS2 wasn't compiled "
                    "with the cmake flag -DADIOS2_USE_ENDIAN_REVERSE=ON "
                    "explicitly, in call to Open\n");
            }
            const size_t pos_last_step = preMetadataIndexFileSize - 64;
            position = pos_last_step;
            const uint64_t lastStep = helper::ReadValue<uint64_t>(
                preMetadataIndex.m_Buffer, position, IsLittleEndian);
            // std::cout << "last step of previous run is: " << lastStep <<
            // std::endl;
            m_BP4Serializer.m_MetadataSet.TimeStep +=
                static_cast<uint32_t>(lastStep);
            m_BP4Serializer.m_MetadataSet.CurrentStep += lastStep;
            // std::cout << "TimeStep is: " <<
            // m_BP4Serializer.m_MetadataSet.TimeStep << std::endl; std::cout <<
            // "CurrentStep is: " << m_BP4Serializer.m_MetadataSet.CurrentStep
            // << std::endl;

            if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
            {
                m_BP4Serializer.m_PreDataFileLength =
                    m_FileDataManager.GetFileSize(0);

                // std::cout << "size of existing data file: " <<
                // m_BP4Serializer.m_PreDataFileLength << std::endl;
            }

            if (m_BP4Serializer.m_RankMPI == 0)
            {
                // Set the flag in the header of metadata index table to 0 again
                // to indicate a new run begins
                UpdateActiveFlag(true);

                // Get the size of existing metadata file
                m_BP4Serializer.m_PreMetadataFileLength =
                    m_FileMetadataManager.GetFileSize(0);
                // std::cout << "size of existing metadata file: " <<
                // m_BP4Serializer.m_PreMetadataFileLength << std::endl;
            }
        }
    }

    if (m_BP4Serializer.m_PreDataFileLength == 0)
    {
        /* This is a new file.
         * Make headers in data buffer and metadata buffer
         */
        if (m_BP4Serializer.m_RankMPI == 0)
        {
            m_BP4Serializer.MakeHeader(m_BP4Serializer.m_Metadata, "Metadata",
                                       false);
        }
        if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
        {
            m_BP4Serializer.MakeHeader(m_BP4Serializer.m_Data, "Data", false);
        }
    }

    m_BP4Serializer.PutProcessGroupIndex(
        m_IO.m_Name, m_IO.m_HostLanguage,
        m_FileDataManager.GetTransportsTypes());
}

void BP4Writer::DoFlush(const bool isFinal, const int transportIndex)
{
    if (m_BP4Serializer.m_Aggregator.m_IsActive)
    {
        AggregateWriteData(isFinal, transportIndex);
    }
    else
    {
        WriteData(isFinal, transportIndex);
    }
}

void BP4Writer::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Writer::Close");
    if (m_BP4Serializer.m_DeferredVariables.size() > 0)
    {
        PerformPuts();

        // DoFlush(false, transportIndex);

        // if (m_BP4Serializer.m_CollectiveMetadata &&
        //     m_FileDataManager.AllTransportsClosed())
        // {
        //     WriteCollectiveMetadataFile(false);
        // }
    }

    DoFlush(true, transportIndex);

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.CloseFiles(transportIndex);
    }

    if (m_BP4Serializer.m_CollectiveMetadata &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteCollectiveMetadataFile(true);
    }

    if (m_BP4Serializer.m_Profiler.IsActive &&
        m_FileDataManager.AllTransportsClosed())
    {
        // std::cout << "write profiling file!" << std::endl;
        WriteProfilingJSONFile();
    }
    if (m_BP4Serializer.m_Aggregator.m_IsActive)
    {
        m_BP4Serializer.m_Aggregator.Close();
    }

    if (m_BP4Serializer.m_RankMPI == 0)
    {
        // close metadata file
        m_FileMetadataManager.CloseFiles();

        // close metadata index file
        m_FileMetadataIndexManager.CloseFiles();
    }
}

void BP4Writer::WriteProfilingJSONFile()
{
    TAU_SCOPED_TIMER("BP4Writer::WriteProfilingJSONFile");
    auto transportTypes = m_FileDataManager.GetTransportsTypes();
    auto transportProfilers = m_FileDataManager.GetTransportsProfilers();

    auto transportTypesMD = m_FileMetadataManager.GetTransportsTypes();
    auto transportProfilersMD = m_FileMetadataManager.GetTransportsProfilers();

    transportTypes.insert(transportTypes.end(), transportTypesMD.begin(),
                          transportTypesMD.end());

    transportProfilers.insert(transportProfilers.end(),
                              transportProfilersMD.begin(),
                              transportProfilersMD.end());

    const std::string lineJSON(m_BP4Serializer.GetRankProfilingJSON(
                                   transportTypes, transportProfilers) +
                               ",\n");

    const std::vector<char> profilingJSON(
        m_BP4Serializer.AggregateProfilingJSON(lineJSON));

    if (m_BP4Serializer.m_RankMPI == 0)
    {
        // std::cout << "write profiling file!" << std::endl;
        transport::FileFStream profilingJSONStream(m_MPIComm, m_DebugMode);
        auto bpBaseNames = m_BP4Serializer.GetBPBaseNames({m_Name});
        profilingJSONStream.Open(bpBaseNames[0] + "/profiling.json",
                                 Mode::Write);
        profilingJSONStream.Write(profilingJSON.data(), profilingJSON.size());
        profilingJSONStream.Close();
    }
}

/*write the content of metadata index file*/
void BP4Writer::PopulateMetadataIndexFileContent(
    BufferSTL &b, const uint64_t currentStep, const uint64_t mpirank,
    const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
    const uint64_t currentTimeStamp)
{
    TAU_SCOPED_TIMER("BP4Writer::PopulateMetadataIndexFileContent");
    auto &buffer = b.m_Buffer;
    auto &position = b.m_Position;
    auto &absolutePosition = b.m_AbsolutePosition;
    helper::CopyToBuffer(buffer, position, &currentStep);
    helper::CopyToBuffer(buffer, position, &mpirank);
    helper::CopyToBuffer(buffer, position, &pgIndexStart);
    helper::CopyToBuffer(buffer, position, &variablesIndexStart);
    helper::CopyToBuffer(buffer, position, &attributesIndexStart);
    helper::CopyToBuffer(buffer, position, &currentStepEndPos);
    helper::CopyToBuffer(buffer, position, &currentTimeStamp);
    position += 8;
}

void BP4Writer::UpdateActiveFlag(const bool active)
{
    const char activeChar = (active ? '\1' : '\0');
    m_FileMetadataIndexManager.WriteFileAt(
        &activeChar, 1, m_BP4Serializer.m_ActiveFlagPosition, 0);
    m_FileMetadataIndexManager.FlushFiles();
    m_FileMetadataIndexManager.SeekToFileEnd();
}

void BP4Writer::WriteCollectiveMetadataFile(const bool isFinal)
{

    TAU_SCOPED_TIMER("BP4Writer::WriteCollectiveMetadataFile");

    if (isFinal && m_BP4Serializer.m_MetadataSet.DataPGCount == 0)
    {
        // If data pg count is zero, it means all metadata
        // has already been written, don't need to write it again.

        if (m_BP4Serializer.m_RankMPI == 0)
        {
            // But the flag in the header of metadata index table needs to be
            // modified to indicate current run is over.
            UpdateActiveFlag(false);
        }
        return;
    }
    m_BP4Serializer.AggregateCollectiveMetadata(
        m_MPIComm, m_BP4Serializer.m_Metadata, true);

    if (m_BP4Serializer.m_RankMPI == 0)
    {

        m_FileMetadataManager.WriteFiles(
            m_BP4Serializer.m_Metadata.m_Buffer.data(),
            m_BP4Serializer.m_Metadata.m_Position);
        m_FileMetadataManager.FlushFiles();

        /*record the starting position of indices in metadata file*/
        const uint64_t pgIndexStartMetadataFile =
            m_BP4Serializer.m_MetadataSet.pgIndexStart +
            m_BP4Serializer.m_MetadataSet.metadataFileLength +
            m_BP4Serializer.m_PreMetadataFileLength;
        const uint64_t varIndexStartMetadataFile =
            m_BP4Serializer.m_MetadataSet.varIndexStart +
            m_BP4Serializer.m_MetadataSet.metadataFileLength +
            m_BP4Serializer.m_PreMetadataFileLength;
        const uint64_t attrIndexStartMetadataFile =
            m_BP4Serializer.m_MetadataSet.attrIndexStart +
            m_BP4Serializer.m_MetadataSet.metadataFileLength +
            m_BP4Serializer.m_PreMetadataFileLength;
        size_t currentStepEndPos =
            m_BP4Serializer.m_MetadataSet.metadataFileLength +
            m_BP4Serializer.m_Metadata.m_Position +
            m_BP4Serializer.m_PreMetadataFileLength;

        BufferSTL metadataIndex;
        metadataIndex.Resize(128, "BP4 Index Table Entry");

        uint64_t currentStep;
        if (isFinal && m_BP4Serializer.m_MetadataSet.DataPGCount > 0)
        {
            // Not run with BeginStep() and EndStep().
            // Only one step of metadata is generated at close.
            currentStep = m_BP4Serializer.m_MetadataSet.TimeStep;
        }
        else
        {
            currentStep = m_BP4Serializer.m_MetadataSet.TimeStep -
                          1; // The current TimeStep has already been increased
                             // by 1 at this point, so decrease it by 1
        }

        // std::cout << "currentStep: " << currentStep << std::endl;

        if (currentStep == 1) // TimeStep starts from 1
        {
            m_BP4Serializer.MakeHeader(metadataIndex, "Index Table", true);
        }

        std::time_t currentTimeStamp = std::time(nullptr);

        PopulateMetadataIndexFileContent(
            metadataIndex, currentStep, m_BP4Serializer.m_RankMPI,
            pgIndexStartMetadataFile, varIndexStartMetadataFile,
            attrIndexStartMetadataFile, currentStepEndPos, currentTimeStamp);

        m_FileMetadataIndexManager.WriteFiles(metadataIndex.m_Buffer.data(),
                                              metadataIndex.m_Position);
        m_FileMetadataIndexManager.FlushFiles();

        m_BP4Serializer.m_MetadataSet.metadataFileLength +=
            m_BP4Serializer.m_Metadata.m_Position;

        if (isFinal)
        {
            // Only one step of metadata is generated at close.
            // The flag in the header of metadata index table
            // needs to be modified to indicate current run is over.
            UpdateActiveFlag(false);
        }
    }
    /*Clear the local indices buffer at the end of each step*/
    m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Metadata, true);
    m_BP4Serializer.ResetIndicesBuffer();
}

void BP4Writer::WriteData(const bool isFinal, const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Writer::WriteData");
    size_t dataSize; // = m_BP4Serializer.m_Data.m_Position;

    // write data without footer
    if (isFinal)
    {
        dataSize = m_BP4Serializer.CloseData(m_IO);
    }
    else
    {
        dataSize = m_BP4Serializer.CloseStream(m_IO, false);
    }

    m_FileDataManager.WriteFiles(m_BP4Serializer.m_Data.m_Buffer.data(),
                                 dataSize, transportIndex);

    m_FileDataManager.FlushFiles(transportIndex);
}

void BP4Writer::AggregateWriteData(const bool isFinal, const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Writer::AggregateWriteData");
    m_BP4Serializer.CloseStream(m_IO, false);

    // async?
    for (int r = 0; r < m_BP4Serializer.m_Aggregator.m_Size; ++r)
    {
        std::vector<std::vector<MPI_Request>> dataRequests =
            m_BP4Serializer.m_Aggregator.IExchange(m_BP4Serializer.m_Data, r);

        std::vector<std::vector<MPI_Request>> absolutePositionRequests =
            m_BP4Serializer.m_Aggregator.IExchangeAbsolutePosition(
                m_BP4Serializer.m_Data, r);

        if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
        {
            const BufferSTL &bufferSTL =
                m_BP4Serializer.m_Aggregator.GetConsumerBuffer(
                    m_BP4Serializer.m_Data);
            if (bufferSTL.m_Position > 0)
            {
                m_FileDataManager.WriteFiles(bufferSTL.m_Buffer.data(),
                                             bufferSTL.m_Position,
                                             transportIndex);

                m_FileDataManager.FlushFiles(transportIndex);
            }
        }

        m_BP4Serializer.m_Aggregator.WaitAbsolutePosition(
            absolutePositionRequests, r);

        m_BP4Serializer.m_Aggregator.Wait(dataRequests, r);
        m_BP4Serializer.m_Aggregator.SwapBuffers(r);
    }

    m_BP4Serializer.UpdateOffsetsInMetadata();

    if (isFinal) // Write metadata footer
    {
        m_BP4Serializer.m_Aggregator.Close();
    }

    m_BP4Serializer.m_Aggregator.ResetBuffers();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
