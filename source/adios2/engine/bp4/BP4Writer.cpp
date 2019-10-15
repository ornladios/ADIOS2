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
                     helper::Comm comm)
: Engine("BP4Writer", io, name, mode, std::move(comm)),
  m_BP4Serializer(m_Comm, m_DebugMode), m_FileDataManager(m_Comm, m_DebugMode),
  m_FileMetadataManager(m_Comm, m_DebugMode),
  m_FileMetadataIndexManager(m_Comm, m_DebugMode)
{
    TAU_SCOPED_TIMER("BP4Writer::Open");
    m_IO.m_ReadStreaming = false;
    m_EndMessage = " in call to IO Open BP4Writer " + m_Name + "\n";
    Init();
}

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
        PerformPutCommon(variable);                                            \
    }

        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
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
    const size_t flushStepsCount = m_BP4Serializer.m_Parameters.FlushStepsCount;

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

    if (m_BP4Serializer.m_Parameters.CollectiveMetadata)
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
    void BP4Writer::DoPut(Variable<T> &variable,                               \
                          typename Variable<T>::Span &span,                    \
                          const size_t bufferID, const T &value)               \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Writer::Put");                                    \
        PutCommon(variable, span, bufferID, value);                            \
    }

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T)                                                        \
    void BP4Writer::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, variable.SetBlockInfo(data, CurrentStep()));   \
        variable.m_BlocksInfo.pop_back();                                      \
    }                                                                          \
    void BP4Writer::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP4Writer::InitParameters()
{
    m_BP4Serializer.Init(m_IO.m_Parameters, "in call to BP4::Open to write");
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

    m_BP4Serializer.m_Profiler.Start("mkdir");
    m_FileDataManager.MkDirsBarrier(bpSubStreamNames,
                                    m_BP4Serializer.m_Parameters.NodeLocal);
    m_BP4Serializer.m_Profiler.Stop("mkdir");

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        if (m_BP4Serializer.m_Parameters.AsyncTasks)
        {
            m_FutureOpenFiles = m_FileDataManager.OpenFilesAsync(
                bpSubStreamNames, m_OpenMode, m_IO.m_TransportsParameters,
                m_BP4Serializer.m_Profiler.m_IsActive);
        }
        else
        {
            m_FileDataManager.OpenFiles(bpSubStreamNames, m_OpenMode,
                                        m_IO.m_TransportsParameters,
                                        m_BP4Serializer.m_Profiler.m_IsActive);
        }
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
                                        m_BP4Serializer.m_Profiler.m_IsActive);

        std::vector<std::string> metadataIndexFileNames =
            m_BP4Serializer.GetBPMetadataIndexFileNames(transportsNames);

        m_FileMetadataIndexManager.OpenFiles(
            metadataIndexFileNames, m_OpenMode, m_IO.m_TransportsParameters,
            m_BP4Serializer.m_Profiler.m_IsActive);

        if (m_OpenMode != Mode::Append ||
            m_FileMetadataIndexManager.GetFileSize(0) == 0)
        {
            /* Prepare header and write now to Index Table indicating
             * the start of streaming */
            m_BP4Serializer.MakeHeader(m_BP4Serializer.m_MetadataIndex,
                                       "Index Table", true);

            m_FileMetadataIndexManager.WriteFiles(
                m_BP4Serializer.m_MetadataIndex.m_Buffer.data(),
                m_BP4Serializer.m_MetadataIndex.m_Position);
            m_FileMetadataIndexManager.FlushFiles();
            /* clear the metadata index buffer*/
            m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_MetadataIndex, true);
        }
        else
        {
            /* Update header to indicate re-start of streaming */
            UpdateActiveFlag(true);
        }
    }
}

void BP4Writer::InitBPBuffer()
{
    if (m_OpenMode == Mode::Append)
    {
        // throw std::invalid_argument(
        //    "ADIOS2: OpenMode Append hasn't been implemented, yet");
        // TODO: Get last pg timestep and update timestep counter in
        format::BufferSTL preMetadataIndex;
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
        m_Comm.BroadcastVector(preMetadataIndex.m_Buffer);
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
                    "with the cmake flag -DADIOS2_USE_Endian_Reverse=ON "
                    "explicitly, in call to Open\n");
            }
            const size_t pos_last_step = preMetadataIndexFileSize - 64;
            position = pos_last_step;
            const uint64_t lastStep = helper::ReadValue<uint64_t>(
                preMetadataIndex.m_Buffer, position, IsLittleEndian);
            m_BP4Serializer.m_MetadataSet.TimeStep +=
                static_cast<uint32_t>(lastStep);
            m_BP4Serializer.m_MetadataSet.CurrentStep += lastStep;

            if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
            {
                if (m_FutureOpenFiles.valid())
                {
                    m_FutureOpenFiles.get();
                }
                m_BP4Serializer.m_PreDataFileLength =
                    m_FileDataManager.GetFileSize(0);
            }

            if (m_BP4Serializer.m_RankMPI == 0)
            {
                // Set the flag in the header of metadata index table to 0 again
                // to indicate a new run begins
                UpdateActiveFlag(true);

                // Get the size of existing metadata file
                m_BP4Serializer.m_PreMetadataFileLength =
                    m_FileMetadataManager.GetFileSize(0);
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
    }

    DoFlush(true, transportIndex);

    if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
    {
        m_FileDataManager.CloseFiles(transportIndex);
    }

    if (m_BP4Serializer.m_Parameters.CollectiveMetadata &&
        m_FileDataManager.AllTransportsClosed())
    {
        WriteCollectiveMetadataFile(true);
    }

    if (m_BP4Serializer.m_Profiler.m_IsActive &&
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
        transport::FileFStream profilingJSONStream(m_Comm, m_DebugMode);
        auto bpBaseNames = m_BP4Serializer.GetBPBaseNames({m_Name});
        profilingJSONStream.Open(bpBaseNames[0] + "/profiling.json",
                                 Mode::Write);
        profilingJSONStream.Write(profilingJSON.data(), profilingJSON.size());
        profilingJSONStream.Close();
    }
}

/*write the content of metadata index file*/
void BP4Writer::PopulateMetadataIndexFileContent(
    format::BufferSTL &b, const uint64_t currentStep, const uint64_t mpirank,
    const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
    const uint64_t currentTimeStamp)
{
    TAU_SCOPED_TIMER("BP4Writer::PopulateMetadataIndexFileContent");
    auto &buffer = b.m_Buffer;
    auto &position = b.m_Position;
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
            // But the flag in the header of metadata index table needs to
            // be modified to indicate current run is over.
            UpdateActiveFlag(false);
        }
        return;
    }
    m_BP4Serializer.AggregateCollectiveMetadata(
        m_Comm, m_BP4Serializer.m_Metadata, true);

    if (m_BP4Serializer.m_RankMPI == 0)
    {

        m_FileMetadataManager.WriteFiles(
            m_BP4Serializer.m_Metadata.m_Buffer.data(),
            m_BP4Serializer.m_Metadata.m_Position);
        m_FileMetadataManager.FlushFiles();

        std::time_t currentTimeStamp = std::time(nullptr);

        std::vector<size_t> timeSteps;
        timeSteps.reserve(
            m_BP4Serializer.m_MetadataIndexTable[m_BP4Serializer.m_RankMPI]
                .size());
        for (auto const &pair :
             m_BP4Serializer.m_MetadataIndexTable[m_BP4Serializer.m_RankMPI])
        {
            timeSteps.push_back(pair.first);
        }
        std::sort(timeSteps.begin(), timeSteps.end());

        size_t rowsInMetadataIndexTable = timeSteps.size() + 1;
        m_BP4Serializer.m_MetadataIndex.Resize(rowsInMetadataIndexTable * 64,
                                               "BP4 Index Table");
        for (auto const &t : timeSteps)
        {
            /*if (t == 1)
            {
                m_BP4Serializer.MakeHeader(m_BP4Serializer.m_MetadataIndex,
                                           "Index Table", true);
            }*/
            const uint64_t pgIndexStartMetadataFile =
                m_BP4Serializer
                    .m_MetadataIndexTable[m_BP4Serializer.m_RankMPI][t][0] +
                m_BP4Serializer.m_MetadataSet.MetadataFileLength +
                m_BP4Serializer.m_PreMetadataFileLength;
            const uint64_t varIndexStartMetadataFile =
                m_BP4Serializer
                    .m_MetadataIndexTable[m_BP4Serializer.m_RankMPI][t][1] +
                m_BP4Serializer.m_MetadataSet.MetadataFileLength +
                m_BP4Serializer.m_PreMetadataFileLength;
            const uint64_t attrIndexStartMetadataFile =
                m_BP4Serializer
                    .m_MetadataIndexTable[m_BP4Serializer.m_RankMPI][t][2] +
                m_BP4Serializer.m_MetadataSet.MetadataFileLength +
                m_BP4Serializer.m_PreMetadataFileLength;
            const uint64_t currentStepEndPosMetadataFile =
                m_BP4Serializer
                    .m_MetadataIndexTable[m_BP4Serializer.m_RankMPI][t][3] +
                m_BP4Serializer.m_MetadataSet.MetadataFileLength +
                m_BP4Serializer.m_PreMetadataFileLength;
            PopulateMetadataIndexFileContent(
                m_BP4Serializer.m_MetadataIndex, t, m_BP4Serializer.m_RankMPI,
                pgIndexStartMetadataFile, varIndexStartMetadataFile,
                attrIndexStartMetadataFile, currentStepEndPosMetadataFile,
                currentTimeStamp);
        }

        m_FileMetadataIndexManager.WriteFiles(
            m_BP4Serializer.m_MetadataIndex.m_Buffer.data(),
            m_BP4Serializer.m_MetadataIndex.m_Position);
        m_FileMetadataIndexManager.FlushFiles();

        m_BP4Serializer.m_MetadataSet.MetadataFileLength +=
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

    /* clear the metadata index buffer*/
    m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_MetadataIndex, true);

    /* reset the metadata index table*/
    m_BP4Serializer.ResetMetadataIndexTable();
    m_BP4Serializer.ResetAllIndices();
}

void BP4Writer::WriteData(const bool isFinal, const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Writer::WriteData");
    size_t dataSize;

    // write data without footer
    if (isFinal)
    {
        dataSize = m_BP4Serializer.CloseData(m_IO);
    }
    else
    {
        dataSize = m_BP4Serializer.CloseStream(m_IO, false);
    }

    if (m_FutureOpenFiles.valid())
    {
        m_FutureOpenFiles.get();
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
        aggregator::MPIAggregator::ExchangeRequests dataRequests =
            m_BP4Serializer.m_Aggregator.IExchange(m_BP4Serializer.m_Data, r);

        aggregator::MPIAggregator::ExchangeAbsolutePositionRequests
            absolutePositionRequests =
                m_BP4Serializer.m_Aggregator.IExchangeAbsolutePosition(
                    m_BP4Serializer.m_Data, r);

        if (m_BP4Serializer.m_Aggregator.m_IsConsumer)
        {
            const format::Buffer &bufferSTL =
                m_BP4Serializer.m_Aggregator.GetConsumerBuffer(
                    m_BP4Serializer.m_Data);
            if (bufferSTL.m_Position > 0)
            {
                if (m_FutureOpenFiles.valid())
                {
                    m_FutureOpenFiles.get();
                }

                m_FileDataManager.WriteFiles(
                    bufferSTL.Data(), bufferSTL.m_Position, transportIndex);

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

#define declare_type(T, L)                                                     \
    T *BP4Writer::DoBufferData_##L(const size_t payloadPosition,               \
                                   const size_t bufferID) noexcept             \
    {                                                                          \
        return BufferDataCommon<T>(payloadPosition, bufferID);                 \
    }

ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

} // end namespace engine
} // end namespace core
} // end namespace adios2
