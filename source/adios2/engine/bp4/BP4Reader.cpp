/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Reader.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Reader.h"
#include "BP4Reader.tcc"

#include "adios2/helper/adiosFunctions.h" // MPI BroadcastVector
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#include <errno.h>
#include <limits>

namespace adios2
{
namespace core
{
namespace engine
{

BP4Reader::BP4Reader(IO &io, const std::string &name, const Mode mode,
                     MPI_Comm mpiComm)
: Engine("BP4Reader", io, name, mode, mpiComm),
  m_BP4Deserializer(mpiComm, m_DebugMode),
  m_MDFileManager(mpiComm, m_DebugMode),
  m_DataFileManager(mpiComm, m_DebugMode),
  m_MDIndexFileManager(mpiComm, m_DebugMode)
{
    TAU_SCOPED_TIMER("BP4Reader::Open");
    Init();
}

StepStatus BP4Reader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    TAU_SCOPED_TIMER("BP4Reader::BeginStep");
    if (m_DebugMode)
    {
        if (mode != StepMode::Read)
        {
            throw std::invalid_argument("ERROR: mode is not supported yet, "
                                        "only Read is valid for "
                                        "engine BP4Reader, in call to "
                                        "BeginStep\n");
        }

        if (!m_BP4Deserializer.m_DeferredVariables.empty())
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
    StepStatus status = StepStatus::OK;

    if (m_CurrentStep >= m_BP4Deserializer.m_MetadataSet.StepsCount)
    {
        status = CheckForNewSteps(timeoutSeconds);
    }

    // This should be after getting new steps
    m_IO.m_EngineStep = m_CurrentStep;

    if (status == StepStatus::OK)
    {
        m_IO.ResetVariablesStepSelection(false,
                                         "in call to BP4 Reader BeginStep");
    }

    return status;
}

size_t BP4Reader::CurrentStep() const { return m_CurrentStep; }

void BP4Reader::EndStep()
{
    TAU_SCOPED_TIMER("BP4Reader::EndStep");
    PerformGets();
}

void BP4Reader::PerformGets()
{
    TAU_SCOPED_TIMER("BP4Reader::PerformGets");
    if (m_BP4Deserializer.m_DeferredVariables.empty())
    {
        return;
    }

    for (const std::string &name : m_BP4Deserializer.m_DeferredVariables)
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
            m_BP4Deserializer.SetVariableBlockInfo(variable, blockInfo);       \
        }                                                                      \
        ReadVariableBlocks(variable);                                          \
        variable.m_BlocksInfo.clear();                                         \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    m_BP4Deserializer.m_DeferredVariables.clear();
}

// PRIVATE
void BP4Reader::Init()
{
    if (m_DebugMode)
    {
        if (m_OpenMode != Mode::Read)
        {
            throw std::invalid_argument("ERROR: BPFileReader only "
                                        "supports OpenMode::Read from" +
                                        m_Name + " " + m_EndMessage);
        }
    }

    m_BP4Deserializer.InitParameters(m_IO.m_Parameters);
    InitTransports();
    InitBuffer();
}

void BP4Reader::OpenFiles()
{
    /* Do a collective wait for the file(s) to appear within timeout.
       Make sure every process comes to the same conclusion */
    float timeoutSeconds = m_BP4Deserializer.m_OpenTimeoutSecs;

    // set poll to 1/100 of timeout
    uint64_t pollTime_ms =
        static_cast<uint64_t>((timeoutSeconds * 1000.0f) / 100);
    if (pollTime_ms < 1000)
    {
        pollTime_ms = 1000; // min 1 second polling time
    }
    if (pollTime_ms > 10000)
    {
        pollTime_ms = 10000; // max 10 seconds polling time
    }

    /* Poll */
    double waited = 0.0;
    double startTime, endTime;
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    std::string lasterrmsg;

    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        while (waited <= timeoutSeconds)
        {
            startTime = MPI_Wtime();
            try
            {
                errno = 0;
                const bool profile = m_BP4Deserializer.m_Profiler.IsActive;
                /* Open the metadata index table */
                const std::string metadataIndexFile(
                    m_BP4Deserializer.GetBPMetadataIndexFileName(m_Name));
                m_MDIndexFileManager.OpenFiles(
                    {metadataIndexFile}, adios2::Mode::Read,
                    m_IO.m_TransportsParameters, profile);

                /* Open the metadata file */
                const std::string metadataFile(
                    m_BP4Deserializer.GetBPMetadataFileName(m_Name));

                m_MDFileManager.OpenFiles({metadataFile}, adios2::Mode::Read,
                                          m_IO.m_TransportsParameters, profile);
                flag = 0; // found file
                break;
            }
            catch (std::ios_base::failure &e)
            {
                lasterrmsg = std::string(e.what());
                if (errno == ENOENT)
                {
                    flag = 1; // timeout
                }
                else
                {
                    flag = 2; // fatal error
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(pollTime_ms));
            endTime = MPI_Wtime();
            waited += endTime - startTime;
        }
    }

    flag = helper::BroadcastValue(flag, m_MPIComm, 0);
    if (flag == 2)
    {
        if (m_BP4Deserializer.m_RankMPI == 0 && !lasterrmsg.empty())
        {
            throw std::ios_base::failure("ERROR: File " + m_Name +
                                         " cannot be opened: " + lasterrmsg);
        }
        else
        {
            throw std::ios_base::failure("File " + m_Name +
                                         " cannot be opened");
        }
    }
    else if (flag == 1)
    {
        if (m_BP4Deserializer.m_RankMPI == 0)
        {
            throw std::ios_base::failure(
                "ERROR: File " + m_Name +
                " could not be found within timeout: " + lasterrmsg);
        }
        else
        {
            throw std::ios_base::failure("ERROR: File " + m_Name +
                                         " could not be found within timeout");
        }
    }

    /* At this point we may have an empty index table.
     * The writer has created the file but has not produced any data yet.
     * We need to wait within the timeout for the data to arrive
     * Header size = 64 bytes, each record is 64 bytes
     */
    flag = 1; // timeout
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        while (waited <= timeoutSeconds)
        {
            startTime = MPI_Wtime();
            const size_t idxFileSize = m_MDIndexFileManager.GetFileSize(0);
            if (idxFileSize > 63)
            {
                flag = 0; // we have data
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(pollTime_ms));
            endTime = MPI_Wtime();
            waited += endTime - startTime;
        }
    }

    flag = helper::BroadcastValue(flag, m_MPIComm, 0);
    if (flag == 1)
    {
        throw std::runtime_error("ERROR: File " + m_Name +
                                 " was found but has not contained data within "
                                 "the specified timeout of " +
                                 std::to_string(timeoutSeconds) + " seconds.");
    }
}

void BP4Reader::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    OpenFiles();
}

void BP4Reader::InitBuffer()
{
    // Put all metadata in buffer
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        /* Read metadata index table into memory */
        const size_t metadataIndexFileSize =
            m_MDIndexFileManager.GetFileSize(0);
        m_BP4Deserializer.m_MetadataIndex.Resize(
            metadataIndexFileSize, "allocating metadata index buffer, "
                                   "in call to BPFileReader Open");
        m_MDIndexFileManager.ReadFile(
            m_BP4Deserializer.m_MetadataIndex.m_Buffer.data(),
            metadataIndexFileSize);

        m_MDIndexFileProcessedSize = metadataIndexFileSize;

        /* Read metadata file into memory */
        const size_t fileSize = m_MDFileManager.GetFileSize(0);
        m_BP4Deserializer.m_Metadata.Resize(
            fileSize, "allocating metadata buffer, in call to BP4Reader Open");

        m_MDFileManager.ReadFile(m_BP4Deserializer.m_Metadata.m_Buffer.data(),
                                 fileSize);
    }
    // broadcast buffer to all ranks from zero
    helper::BroadcastVector(m_BP4Deserializer.m_Metadata.m_Buffer, m_MPIComm);

    // broadcast metadata index buffer to all ranks from zero
    helper::BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer,
                            m_MPIComm);

    /* Parse metadata index table */
    m_BP4Deserializer.ParseMetadataIndex(m_BP4Deserializer.m_MetadataIndex);

    // fills IO with Variables and Attributes
    m_MDFileProcessedSize =
        m_BP4Deserializer.ParseMetadata(m_BP4Deserializer.m_Metadata, *this);
    /* m_MDFileProcessedSize is the position in the buffer where processing
     * ends. The processing is controlled by the number of records in the Index,
     * which may be less than the actual entries in the metadata in a streaming
     * situation (where writer has just written metadata for step
     * K+1,...,K+L while the index contains K steps when the reader looks at
     * it).
     *
     * In ProcessMetadataForNewSteps(), we will re-read the metadata which
     * is in the buffer but has not been processed yet.
     */
}

size_t BP4Reader::UpdateBuffer()
{
    std::vector<size_t> sizes(2, 0);
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        const size_t idxFileSize = m_MDIndexFileManager.GetFileSize(0);
        if (idxFileSize > m_MDIndexFileProcessedSize)
        {
            const size_t newIdxSize = idxFileSize - m_MDIndexFileProcessedSize;
            if (m_BP4Deserializer.m_MetadataIndex.m_Buffer.size() < newIdxSize)
            {
                m_BP4Deserializer.m_MetadataIndex.Resize(
                    newIdxSize, "re-allocating metadata index buffer, in "
                                "call to BP4Reader::BeginStep/UpdateBuffer");
            }
            m_BP4Deserializer.m_MetadataIndex.m_Position = 0;
            m_MDIndexFileManager.ReadFile(
                m_BP4Deserializer.m_MetadataIndex.m_Buffer.data(), newIdxSize,
                m_MDIndexFileProcessedSize);

            sizes[0] = newIdxSize;

            /* Read corresponding new metadata (throwing away the old)
             * There may be unprocessed entries in the metadata if the index
             * had less steps than the metadata file at the last read.
             * Those steps are read again here, starting in the beginning of
             * the buffer now.
             */
            const size_t fileSize = m_MDFileManager.GetFileSize(0);
            const size_t newMDSize = fileSize - m_MDFileProcessedSize;
            if (m_BP4Deserializer.m_Metadata.m_Buffer.size() < newMDSize)
            {
                m_BP4Deserializer.m_Metadata.Resize(
                    newMDSize, "allocating metadata buffer, in call to "
                               "BP4Reader Open");
            }
            m_BP4Deserializer.m_Metadata.m_Position = 0;
            m_MDFileManager.ReadFile(
                m_BP4Deserializer.m_Metadata.m_Buffer.data(), newMDSize,
                m_MDFileProcessedSize);

            sizes[1] = m_MDFileProcessedSize;
        }
    }

    helper::BroadcastVector(sizes, m_MPIComm, 0);
    size_t newIdxSize = sizes[0];

    if (newIdxSize > 0)
    {
        // broadcast buffer to all ranks from zero
        helper::BroadcastVector(m_BP4Deserializer.m_Metadata.m_Buffer,
                                m_MPIComm);

        // broadcast metadata index buffer to all ranks from zero
        helper::BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer,
                                m_MPIComm);

        if (m_BP4Deserializer.m_RankMPI != 0)
        {
            m_MDFileProcessedSize = sizes[1];
            // we need this pointer in Metadata buffer on all processes
            // for parsing it correctly in ProcessMetadataForNewSteps()
        }
    }
    return newIdxSize;
}
void BP4Reader::ProcessMetadataForNewSteps(const size_t newIdxSize)
{
    /* Remove all existing variables from previous steps
       It seems easier than trying to update them */
    m_IO.RemoveAllVariables();

    /* Parse metadata index table (without header) */
    /* We need to skew the index table pointers with the
       size of the already-processed metadata because the memory buffer of
       new metadata starts from 0 */
    m_BP4Deserializer.ParseMetadataIndex(m_BP4Deserializer.m_MetadataIndex,
                                         m_MDFileProcessedSize, false);

    // fills IO with Variables and Attributes (not first step)
    const size_t newProcessedMDSize = m_BP4Deserializer.ParseMetadata(
        m_BP4Deserializer.m_Metadata, *this, false);

    // remember current end position in metadata and index table for next round
    m_MDFileProcessedSize += newProcessedMDSize;
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        m_MDIndexFileProcessedSize += newIdxSize;
    }
}

bool BP4Reader::CheckWriterActive()
{
    size_t flag = 0;
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        std::vector<char> header(64, '\0');
        m_MDIndexFileManager.ReadFile(header.data(), 64, 0, 0);
        bool active = m_BP4Deserializer.ReadActiveFlag(header);
        flag = (active ? 1 : 0);
    }
    flag = helper::BroadcastValue(flag, m_BP4Deserializer.m_MPIComm, 0);
    m_BP4Deserializer.m_WriterIsActive = (flag > 0);
    return m_BP4Deserializer.m_WriterIsActive;
}

StepStatus BP4Reader::CheckForNewSteps(float timeoutSeconds)
{
    /* Do a collective wait for a step within timeout.
       Make sure every reader comes to the same conclusion */
    StepStatus retval = StepStatus::OK;
    bool haveNewStep = false;

    if (timeoutSeconds < 0.0)
    {
        timeoutSeconds = std::numeric_limits<float>::max();
    }

    float pollSecs = m_BP4Deserializer.m_BeginStepPollingFrequencySecs;
    if (pollSecs > timeoutSeconds)
    {
        pollSecs = timeoutSeconds;
    }
    uint64_t pollTime_ms = static_cast<uint64_t>(pollSecs * 1000.f);

    /* Poll */
    double waited = 0.0;
    double startTime, endTime;

    // Hack: processing metadata for multiple new steps only works
    // when pretending not to be in streaming mode
    const bool saveReadStreaming = m_IO.m_ReadStreaming;

    m_IO.m_ReadStreaming = false;
    while (waited < timeoutSeconds && m_BP4Deserializer.m_WriterIsActive)
    {
        startTime = MPI_Wtime();
        size_t newIdxSize = UpdateBuffer();
        if (newIdxSize > 0)
        {
            haveNewStep = true;
            /* we have new metadata in memory. Need to parse it now */
            ProcessMetadataForNewSteps(newIdxSize);
            break;
        }
        if (!CheckWriterActive())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(pollTime_ms));
        endTime = MPI_Wtime();
        waited += endTime - startTime;
    }

    if (!haveNewStep)
    {
        m_IO.m_ReadStreaming = false;
        if (m_BP4Deserializer.m_WriterIsActive)
        {
            retval = StepStatus::NotReady;
        }
        else
        {
            retval = StepStatus::EndOfStream;
        }
    }
    else
    {
        retval = StepStatus::OK;
    }
    m_IO.m_ReadStreaming = saveReadStreaming;

    return retval;
}

#define declare_type(T)                                                        \
    void BP4Reader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::Get");                                    \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP4Reader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::Get");                                    \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP4Reader::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("BP4Reader::Close");
    PerformGets();
    m_DataFileManager.CloseFiles();
    m_MDFileManager.CloseFiles();
}

#define declare_type(T)                                                        \
    std::map<size_t, std::vector<typename Variable<T>::Info>>                  \
    BP4Reader::DoAllStepsBlocksInfo(const Variable<T> &variable) const         \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::AllStepsBlocksInfo");                     \
        return m_BP4Deserializer.AllStepsBlocksInfo(variable);                 \
    }                                                                          \
                                                                               \
    std::vector<std::vector<typename Variable<T>::Info>>                       \
    BP4Reader::DoAllRelativeStepsBlocksInfo(const Variable<T> &variable) const \
    {                                                                          \
        TAU_SCOPED_TIMER("BP3Reader::AllRelativeStepsBlocksInfo");             \
        return m_BP4Deserializer.AllRelativeStepsBlocksInfo(variable);         \
    }                                                                          \
                                                                               \
    std::vector<typename Variable<T>::Info> BP4Reader::DoBlocksInfo(           \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        TAU_SCOPED_TIMER("BP4Reader::BlocksInfo");                             \
        return m_BP4Deserializer.BlocksInfo(variable, step);                   \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

size_t BP4Reader::DoSteps() const
{
    return m_BP4Deserializer.m_MetadataSet.StepsCount;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
