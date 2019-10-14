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

#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#include <chrono>
#include <errno.h>

namespace adios2
{
namespace core
{
namespace engine
{

BP4Reader::BP4Reader(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("BP4Reader", io, name, mode, std::move(comm)),
  m_BP4Deserializer(m_Comm, m_DebugMode), m_MDFileManager(m_Comm, m_DebugMode),
  m_DataFileManager(m_Comm, m_DebugMode),
  m_MDIndexFileManager(m_Comm, m_DebugMode)
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

    // used to inquire for variables in streaming mode
    m_IO.m_ReadStreaming = true;
    StepStatus status = StepStatus::OK;

    if (m_FirstStep)
    {
        if (m_BP4Deserializer.m_MetadataSet.StepsCount == 0)
        {
            status = CheckForNewSteps(Seconds(timeoutSeconds));
        }
    }
    else
    {
        if (m_CurrentStep + 1 >= m_BP4Deserializer.m_MetadataSet.StepsCount)
        {
            status = CheckForNewSteps(Seconds(timeoutSeconds));
        }
    }

    // This should be after getting new steps

    if (status == StepStatus::OK)
    {
        if (m_FirstStep)
        {
            m_FirstStep = false;
        }
        else
        {
            ++m_CurrentStep;
        }

        m_IO.m_EngineStep = m_CurrentStep;
        m_IO.ResetVariablesStepSelection(false,
                                         "in call to BP4 Reader BeginStep");

        // caches attributes for each step
        // if a variable name is a prefix
        // e.g. var  prefix = {var/v1, var/v2, var/v3}
        m_IO.SetPrefixedNames(true);
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

    m_BP4Deserializer.Init(m_IO.m_Parameters, "in call to BP4::Open to write");
    InitTransports();

    /* Do a collective wait for the file(s) to appear within timeout.
       Make sure every process comes to the same conclusion */
    const Seconds timeoutSeconds =
        Seconds(m_BP4Deserializer.m_Parameters.OpenTimeoutSecs);

    // set poll to 1/100 of timeout
    Seconds pollSeconds = timeoutSeconds / 100.0;
    static const auto pollSecondsMin = Seconds(1.0);
    if (pollSeconds < pollSecondsMin)
    {
        pollSeconds = pollSecondsMin;
    }
    static const auto pollSecondsMax = Seconds(10.0);
    if (pollSeconds > pollSecondsMax)
    {
        pollSeconds = pollSecondsMax;
    }

    const TimePoint timeoutInstant =
        std::chrono::steady_clock::now() + timeoutSeconds;

    OpenFiles(timeoutInstant, pollSeconds, timeoutSeconds);
    InitBuffer(timeoutInstant, pollSeconds / 10, timeoutSeconds);
}

void BP4Reader::OpenFiles(const TimePoint &timeoutInstant,
                          const Seconds &pollSeconds,
                          const Seconds &timeoutSeconds)
{
    /* Poll */
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    std::string lasterrmsg;
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        do
        {
            try
            {
                errno = 0;
                const bool profile = m_BP4Deserializer.m_Profiler.m_IsActive;
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

            std::this_thread::sleep_for(pollSeconds);
        } while (std::chrono::steady_clock::now() < timeoutInstant);
    }

    flag = m_Comm.BroadcastValue(flag, 0);
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
     * The writer has created the file but no content may have been stored yet.
     * We need to wait within the timeout for the index header to arrive (from
     * writer's open), so that we don't need to deal with the header later.
     * Zero or more actual steps in the output is fine.
     * Header size = 64 bytes, each record is 64 bytes
     */
    flag = 1; // timeout
    if (m_BP4Deserializer.m_RankMPI == 0)
    {
        do
        {
            const size_t idxFileSize = m_MDIndexFileManager.GetFileSize(0);
            if (idxFileSize > 63)
            {
                flag = 0; // we have at least the header
                break;
            }
            std::this_thread::sleep_for(pollSeconds);
        } while (std::chrono::steady_clock::now() < timeoutInstant);
    }

    flag = m_Comm.BroadcastValue(flag, 0);
    if (flag == 1)
    {
        throw std::ios_base::failure(
            "ERROR: File " + m_Name +
            " was found but has not contained data within "
            "the specified timeout of " +
            std::to_string(timeoutSeconds.count()) + " seconds.");
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
}

uint64_t
MetadataExpectedMinFileSize(const format::BP4Deserializer &m_BP4Deserializer,
                            const std::string &IdxFileName, bool hasHeader)
{
    size_t idxsize = m_BP4Deserializer.m_MetadataIndex.m_Buffer.size();
    if (idxsize % 64 != 0)
    {
        throw std::runtime_error(
            "FATAL CODING ERROR: ADIOS Index file " + IdxFileName +
            " is assumed to always contain n*64 byte-length records. "
            "The file size now is " +
            std::to_string(idxsize) + " bytes.");
    }
    if ((hasHeader && idxsize < 128) || idxsize < 64)
    {
        // no (new) step entry in the index, so no metadata is expected
        return 0;
    }
    uint64_t lastpos = *(uint64_t *)&(
        m_BP4Deserializer.m_MetadataIndex.m_Buffer[idxsize - 24]);
    return lastpos;
}

void BP4Reader::InitBuffer(const TimePoint &timeoutInstant,
                           const Seconds &pollSeconds,
                           const Seconds &timeoutSeconds)
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

        /* Read metadata file into memory but first make sure
         * it has the content that the index table refers to */
        uint64_t expectedMinFileSize =
            MetadataExpectedMinFileSize(m_BP4Deserializer, m_Name, true);
        size_t fileSize = 0;
        do
        {
            fileSize = m_MDFileManager.GetFileSize(0);
            if (fileSize >= expectedMinFileSize)
            {
                break;
            }
            std::this_thread::sleep_for(pollSeconds);
        } while (std::chrono::steady_clock::now() < timeoutInstant);

        if (fileSize >= expectedMinFileSize)
        {
            m_BP4Deserializer.m_Metadata.Resize(
                fileSize,
                "allocating metadata buffer, in call to BP4Reader Open");

            m_MDFileManager.ReadFile(
                m_BP4Deserializer.m_Metadata.m_Buffer.data(), fileSize);
            m_MDIndexFileProcessedSize = metadataIndexFileSize;
        }
        else
        {
            throw std::ios_base::failure(
                "ERROR: File " + m_Name +
                " was found with an index file but md.0 "
                "has not contained enough data within "
                "the specified timeout of " +
                std::to_string(timeoutSeconds.count()) + " seconds.");
        }
    }
    // broadcast buffer to all ranks from zero
    m_Comm.BroadcastVector(m_BP4Deserializer.m_Metadata.m_Buffer);

    // broadcast metadata index buffer to all ranks from zero
    m_Comm.BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer);

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

size_t BP4Reader::UpdateBuffer(const TimePoint &timeoutInstant,
                               const Seconds &pollSeconds)
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

            /* Wait until as much metadata arrives in the file as much
             * is indicated by the existing index entries
             */
            uint64_t expectedMinFileSize =
                MetadataExpectedMinFileSize(m_BP4Deserializer, m_Name, false);
            size_t fileSize = 0;
            do
            {
                fileSize = m_MDFileManager.GetFileSize(0);
                if (fileSize >= expectedMinFileSize)
                {
                    break;
                }
                std::this_thread::sleep_for(pollSeconds);
            } while (std::chrono::steady_clock::now() < timeoutInstant);

            if (fileSize >= expectedMinFileSize)
            {
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

                sizes[0] = newIdxSize;
                sizes[1] = m_MDFileProcessedSize;
            }
        }
    }

    m_Comm.BroadcastVector(sizes, 0);
    size_t newIdxSize = sizes[0];

    if (newIdxSize > 0)
    {
        // broadcast buffer to all ranks from zero
        m_Comm.BroadcastVector(m_BP4Deserializer.m_Metadata.m_Buffer);

        // broadcast metadata index buffer to all ranks from zero
        m_Comm.BroadcastVector(m_BP4Deserializer.m_MetadataIndex.m_Buffer);

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
    size_t idxsize = m_BP4Deserializer.m_MetadataIndex.m_Buffer.size();
    uint64_t lastpos = *(uint64_t *)&(
        m_BP4Deserializer.m_MetadataIndex.m_Buffer[idxsize - 24]);
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
    flag = m_BP4Deserializer.m_Comm.BroadcastValue(flag, 0);
    m_BP4Deserializer.m_WriterIsActive = (flag > 0);
    return m_BP4Deserializer.m_WriterIsActive;
}

StepStatus BP4Reader::CheckForNewSteps(Seconds timeoutSeconds)
{
    /* Do a collective wait for a step within timeout.
       Make sure every reader comes to the same conclusion */
    StepStatus retval = StepStatus::OK;
    bool haveNewStep = false;

    if (timeoutSeconds < Seconds::zero())
    {
        timeoutSeconds = Seconds(999999999); // max 1 billion seconds wait
    }
    const TimePoint timeoutInstant =
        std::chrono::steady_clock::now() + timeoutSeconds;

    auto pollSeconds =
        Seconds(m_BP4Deserializer.m_Parameters.BeginStepPollingFrequencySecs);
    if (pollSeconds > timeoutSeconds)
    {
        pollSeconds = timeoutSeconds;
    }

    /* Poll */

    // Hack: processing metadata for multiple new steps only works
    // when pretending not to be in streaming mode
    const bool saveReadStreaming = m_IO.m_ReadStreaming;

    m_IO.m_ReadStreaming = false;
    while (m_BP4Deserializer.m_WriterIsActive)
    {
        size_t newIdxSize = UpdateBuffer(timeoutInstant, pollSeconds / 10);
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
        std::this_thread::sleep_for(pollSeconds);
        if (std::chrono::steady_clock::now() >= timeoutInstant)
        {
            break;
        }
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
