/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Reader.cpp
 *
 */

#include "BP5Reader.h"
#include "BP5Reader.tcc"

#include <adios2-perfstubs-interface.h>

#include <errno.h>

namespace adios2
{
namespace core
{
namespace engine
{

BP5Reader::BP5Reader(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("BP5Reader", io, name, mode, std::move(comm)), m_MDFileManager(m_Comm),
  m_DataFileManager(m_Comm), m_MDIndexFileManager(m_Comm),
  m_FileMetaMetadataManager(m_Comm), m_ActiveFlagFileManager(m_Comm)
{
    PERFSTUBS_SCOPED_TIMER("BP5Reader::Open");
    Init();
}

BP5Reader::~BP5Reader()
{
    if (m_BP5Deserializer)
        delete m_BP5Deserializer;
}

void BP5Reader::InstallMetadataForTimestep(size_t Step)
{
    size_t pgstart = m_MetadataIndexTable[Step][0];
    size_t Position = pgstart + sizeof(uint64_t); // skip total data size
    size_t MDPosition = Position + 2 * sizeof(uint64_t) * m_WriterCount;
    for (size_t WriterRank = 0; WriterRank < m_WriterCount; WriterRank++)
    {
        // variable metadata for timestep
        size_t ThisMDSize = helper::ReadValue<uint64_t>(
            m_Metadata.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        char *ThisMD = m_Metadata.m_Buffer.data() + MDPosition;
        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            m_BP5Deserializer->InstallMetaData(ThisMD, ThisMDSize, WriterRank,
                                               Step);
        }
        else
        {
            m_BP5Deserializer->InstallMetaData(ThisMD, ThisMDSize, WriterRank);
        }
        MDPosition += ThisMDSize;
    }
    for (size_t WriterRank = 0; WriterRank < m_WriterCount; WriterRank++)
    {
        // attribute metadata for timestep
        size_t ThisADSize = helper::ReadValue<uint64_t>(
            m_Metadata.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        char *ThisAD = m_Metadata.m_Buffer.data() + MDPosition;
        if (ThisADSize > 0)
            m_BP5Deserializer->InstallAttributeData(ThisAD, ThisADSize);
        MDPosition += ThisADSize;
    }
}

StepStatus BP5Reader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    PERFSTUBS_SCOPED_TIMER("BP5Reader::BeginStep");

    if (m_OpenMode == Mode::ReadRandomAccess)
    {
        throw std::logic_error(
            "ERROR: BeginStep called in random access mode\n");
    }
    if (m_BetweenStepPairs)
    {
        throw std::logic_error("ERROR: BeginStep() is called a second time "
                               "without an intervening EndStep()");
    }

    if (mode != StepMode::Read)
    {
        throw std::invalid_argument("ERROR: mode is not supported yet, "
                                    "only Read is valid for "
                                    "engine BP5Reader, in call to "
                                    "BeginStep\n");
    }

    StepStatus status = StepStatus::OK;
    if (m_FirstStep)
    {
        if (m_StepsCount == 0)
        {
            //            status = CheckForNewSteps(Seconds(timeoutSeconds));
        }
    }
    else
    {
        if (m_CurrentStep + 1 >= m_StepsCount)
        {
            // status = CheckForNewSteps(Seconds(timeoutSeconds));
            status = StepStatus::EndOfStream;
        }
    }
    if (status == StepStatus::OK)
    {
        m_BetweenStepPairs = true;
        if (m_FirstStep)
        {
            m_FirstStep = false;
        }
        else
        {
            ++m_CurrentStep;
        }

        m_IO.m_EngineStep = m_CurrentStep;
        //        SstBlock AttributeBlockList =
        //            SstGetAttributeData(m_Input, SstCurrentStep(m_Input));
        //        i = 0;
        //        while (AttributeBlockList && AttributeBlockList[i].BlockData)
        //        {
        //            m_IO.RemoveAllAttributes();
        //            m_BP5Deserializer->InstallAttributeData(
        //                AttributeBlockList[i].BlockData,
        //                AttributeBlockList[i].BlockSize);
        //            i++;
        //        }

        m_BP5Deserializer->SetupForTimestep(m_CurrentStep);

        InstallMetadataForTimestep(m_CurrentStep);
        m_IO.ResetVariablesStepSelection(false,
                                         "in call to BP5 Reader BeginStep");

        // caches attributes for each step
        // if a variable name is a prefix
        // e.g. var  prefix = {var/v1, var/v2, var/v3}
        m_IO.SetPrefixedNames(true);
    }

    return status;
}

size_t BP5Reader::CurrentStep() const { return m_CurrentStep; }

void BP5Reader::EndStep()
{
    if (m_OpenMode == Mode::ReadRandomAccess)
    {
        throw std::logic_error("ERROR: EndStep called in random access mode\n");
    }
    if (!m_BetweenStepPairs)
    {
        throw std::logic_error(
            "ERROR: EndStep() is called without a successful BeginStep()");
    }
    m_BetweenStepPairs = false;
    PERFSTUBS_SCOPED_TIMER("BP5Reader::EndStep");
    PerformGets();
}

void BP5Reader::ReadData(const size_t WriterRank, const size_t Timestep,
                         const size_t StartOffset, const size_t Length,
                         char *Destination)
{
    size_t FlushCount = m_MetadataIndexTable[Timestep][2];
    size_t DataPosPos = m_MetadataIndexTable[Timestep][3];
    size_t SubfileNum = m_WriterToFileMap[WriterRank];

    // check if subfile is already opened
    if (m_DataFileManager.m_Transports.count(SubfileNum) == 0)
    {
        const std::string subFileName = GetBPSubStreamName(
            m_Name, SubfileNum, m_Minifooter.HasSubFiles, true);

        m_DataFileManager.OpenFileID(subFileName, SubfileNum, Mode::Read,
                                     {{"transport", "File"}}, false);
    }

    size_t InfoStartPos =
        DataPosPos + (WriterRank * (2 * FlushCount + 1) * sizeof(uint64_t));
    size_t ThisFlushInfo = InfoStartPos;
    size_t RemainingLength = Length;
    size_t ThisDataPos;
    size_t Offset = StartOffset;
    for (size_t flush = 0; flush < FlushCount; flush++)
    {

        ThisDataPos =
            helper::ReadValue<uint64_t>(m_MetadataIndex.m_Buffer, ThisFlushInfo,
                                        m_Minifooter.IsLittleEndian);
        size_t ThisDataSize =
            helper::ReadValue<uint64_t>(m_MetadataIndex.m_Buffer, ThisFlushInfo,
                                        m_Minifooter.IsLittleEndian);
        if (ThisDataSize > RemainingLength)
            ThisDataSize = RemainingLength;
        m_DataFileManager.ReadFile(Destination, ThisDataSize,
                                   ThisDataPos + Offset, SubfileNum);
        Destination += ThisDataSize;
        RemainingLength -= ThisDataSize;
        Offset = 0;
        if (RemainingLength == 0)
            return;
    }
    ThisDataPos = helper::ReadValue<uint64_t>(
        m_MetadataIndex.m_Buffer, ThisFlushInfo, m_Minifooter.IsLittleEndian);
    m_DataFileManager.ReadFile(Destination, RemainingLength, ThisDataPos,
                               SubfileNum);
}

void BP5Reader::PerformGets()
{
    PERFSTUBS_SCOPED_TIMER("BP5Reader::PerformGets");
    auto ReadRequests = m_BP5Deserializer->GenerateReadRequests();
    // Potentially optimize read requests, make contiguous, etc.
    for (const auto &Req : ReadRequests)
    {
        ReadData(Req.WriterRank, Req.Timestep, Req.StartOffset, Req.ReadLength,
                 Req.DestinationAddr);
    }

    m_BP5Deserializer->FinalizeGets(ReadRequests);
}

// PRIVATE
void BP5Reader::Init()
{
    if ((m_OpenMode != Mode::Read) && (m_OpenMode != Mode::ReadRandomAccess))
    {
        throw std::invalid_argument(
            "ERROR: BPFileReader only "
            "supports OpenMode::Read or OpenMode::ReadRandomAccess from" +
            m_Name + " " + m_EndMessage);
    }

    // if IO was involved in reading before this flag may be true now
    m_IO.m_ReadStreaming = false;

    ParseParams(m_IO, m_Parameters);
    m_ReaderIsRowMajor = (m_IO.m_ArrayOrder == ArrayOrdering::RowMajor);
    InitTransports();

    /* Do a collective wait for the file(s) to appear within timeout.
       Make sure every process comes to the same conclusion */
    const Seconds timeoutSeconds = Seconds(m_Parameters.OpenTimeoutSecs);

    Seconds pollSeconds = Seconds(m_Parameters.BeginStepPollingFrequencySecs);
    if (pollSeconds > timeoutSeconds)
    {
        pollSeconds = timeoutSeconds;
    }

    TimePoint timeoutInstant = Now() + timeoutSeconds;

    OpenFiles(timeoutInstant, pollSeconds, timeoutSeconds);

    /* non-stream reader gets as much steps as available now */
    InitBuffer(timeoutInstant, pollSeconds / 10, timeoutSeconds);
}

bool BP5Reader::SleepOrQuit(const TimePoint &timeoutInstant,
                            const Seconds &pollSeconds)
{
    auto now = Now();
    if (now + pollSeconds >= timeoutInstant)
    {
        return false;
    }
    auto remainderTime = timeoutInstant - now;
    auto sleepTime = pollSeconds;
    if (remainderTime < sleepTime)
    {
        sleepTime = remainderTime;
    }
    std::this_thread::sleep_for(sleepTime);
    return true;
}

size_t BP5Reader::OpenWithTimeout(transportman::TransportMan &tm,
                                  const std::vector<std::string> &fileNames,
                                  const TimePoint &timeoutInstant,
                                  const Seconds &pollSeconds,
                                  std::string &lasterrmsg /*INOUT*/)
{
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    do
    {
        try
        {
            errno = 0;
            const bool profile =
                false; // m_BP4Deserializer.m_Profiler.m_IsActive;
            tm.OpenFiles(fileNames, adios2::Mode::Read,
                         m_IO.m_TransportsParameters, profile);
            flag = 0; // found file
            break;
        }
        catch (std::ios_base::failure &e)
        {
            lasterrmsg =
                std::string("errno=" + std::to_string(errno) + ": " + e.what());
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
    } while (SleepOrQuit(timeoutInstant, pollSeconds));
    return flag;
}

void BP5Reader::OpenFiles(TimePoint &timeoutInstant, const Seconds &pollSeconds,
                          const Seconds &timeoutSeconds)
{
    /* Poll */
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    std::string lasterrmsg;
    if (m_Comm.Rank() == 0)
    {
        /* Open the metadata index table */
        const std::string metadataIndexFile(GetBPMetadataIndexFileName(m_Name));

        flag = OpenWithTimeout(m_MDIndexFileManager, {metadataIndexFile},
                               timeoutInstant, pollSeconds, lasterrmsg);
        if (flag == 0)
        {
            /* Open the metadata file */
            const std::string metadataFile(GetBPMetadataFileName(m_Name));

            /* We found md.idx. If we don't find md.0 immediately  we should
             * wait a little bit hoping for the file system to catch up.
             * This slows down finding the error in file reading mode but
             * it will be more robust in streaming mode
             */
            if (timeoutSeconds == Seconds(0.0))
            {
                timeoutInstant += Seconds(5.0);
            }

            flag = OpenWithTimeout(m_MDFileManager, {metadataFile},
                                   timeoutInstant, pollSeconds, lasterrmsg);
            if (flag != 0)
            {
                /* Close the metadata index table */
                m_MDIndexFileManager.CloseFiles();
            }
            else
            {
                /* Open the metametadata file */
                const std::string metametadataFile(
                    GetBPMetaMetadataFileName(m_Name));

                /* We found md.idx. If we don't find md.0 immediately  we should
                 * wait a little bit hoping for the file system to catch up.
                 * This slows down finding the error in file reading mode but
                 * it will be more robust in streaming mode
                 */
                if (timeoutSeconds == Seconds(0.0))
                {
                    timeoutInstant += Seconds(5.0);
                }

                flag = OpenWithTimeout(m_FileMetaMetadataManager,
                                       {metametadataFile}, timeoutInstant,
                                       pollSeconds, lasterrmsg);
                if (flag != 0)
                {
                    /* Close the metametadata index table */
                    m_MDIndexFileManager.CloseFiles();
                    m_MDFileManager.CloseFiles();
                }
            }
        }
    }

    flag = m_Comm.BroadcastValue(flag, 0);
    if (flag == 2)
    {
        if (m_Comm.Rank() == 0 && !lasterrmsg.empty())
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
        if (m_Comm.Rank() == 0)
        {
            throw std::ios_base::failure(
                "ERROR: File " + m_Name + " could not be found within the " +
                std::to_string(timeoutSeconds.count()) +
                "s timeout: " + lasterrmsg);
        }
        else
        {
            throw std::ios_base::failure(
                "ERROR: File " + m_Name + " could not be found within the " +
                std::to_string(timeoutSeconds.count()) + "s timeout");
        }
    }

    /* At this point we may have an empty index table.
     * The writer has created the file but no content may have been stored yet.
     */
}

Engine::MinVarInfo *BP5Reader::MinBlocksInfo(const VariableBase &Var,
                                             const size_t Step) const
{
    return m_BP5Deserializer->MinBlocksInfo(Var, Step);
}

bool BP5Reader::VariableMinMax(const VariableBase &Var, const size_t Step,
                               Engine::MinMaxStruct &MinMax)
{
    return m_BP5Deserializer->VariableMinMax(Var, Step, MinMax);
}

void BP5Reader::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
}

uint64_t BP5Reader::MetadataExpectedMinFileSize(const std::string &IdxFileName,
                                                bool hasHeader)
{
    size_t cur_idxsize = m_MetadataIndex.m_Buffer.size();
    static constexpr size_t m_MinIndexRecordSize = 3 * sizeof(uint64_t);
    if ((hasHeader && cur_idxsize < m_IndexHeaderSize + m_MinIndexRecordSize) ||
        cur_idxsize < m_MinIndexRecordSize)
    {
        // no (new) step entry in the index, so no metadata is expected
        return 0;
    }
    uint64_t lastpos =
        *(uint64_t *)&(m_MetadataIndex.m_Buffer[cur_idxsize - 24]);
    return lastpos;
}

void BP5Reader::InstallMetaMetaData(format::BufferSTL buffer)
{
    size_t Position = 0;
    while (Position < buffer.m_Buffer.size())
    {
        format::BP5Base::MetaMetaInfoBlock MMI;

        MMI.MetaMetaIDLen = helper::ReadValue<uint64_t>(
            buffer.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        MMI.MetaMetaInfoLen = helper::ReadValue<uint64_t>(
            buffer.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        MMI.MetaMetaID = buffer.Data() + Position;
        MMI.MetaMetaInfo = buffer.Data() + Position + MMI.MetaMetaIDLen;
        m_BP5Deserializer->InstallMetaMetaData(MMI);
        Position += MMI.MetaMetaIDLen + MMI.MetaMetaInfoLen;
    }
}

void BP5Reader::InitBuffer(const TimePoint &timeoutInstant,
                           const Seconds &pollSeconds,
                           const Seconds &timeoutSeconds)
{
    size_t newIdxSize = 0;
    // Put all metadata in buffer
    if (m_Comm.Rank() == 0)
    {
        /* Read metadata index table into memory */
        const size_t metadataIndexFileSize =
            m_MDIndexFileManager.GetFileSize(0);
        if (metadataIndexFileSize > 0)
        {
            m_MetadataIndex.Resize(metadataIndexFileSize,
                                   "allocating metadata index buffer, "
                                   "in call to BPFileReader Open");
            m_MDIndexFileManager.ReadFile(m_MetadataIndex.m_Buffer.data(),
                                          metadataIndexFileSize);

            /* Read metametadata into memory */
            const size_t metametadataFileSize =
                m_FileMetaMetadataManager.GetFileSize(0);
            m_MetaMetadata.Resize(metametadataFileSize,
                                  "allocating metadata index buffer, "
                                  "in call to BPFileReader Open");
            m_FileMetaMetadataManager.ReadFile(m_MetaMetadata.m_Buffer.data(),
                                               metametadataFileSize);

            size_t fileSize = 0;
            fileSize = m_MDFileManager.GetFileSize(0);
#ifdef NOTDEF
            /* Read metadata file into memory but first make sure
             * it has the content that the index table refers to */
            uint64_t expectedMinFileSize =
                MetadataExpectedMinFileSize(m_Name, true);
            do
            {
                fileSize = m_MDFileManager.GetFileSize(0);
                if (fileSize >= expectedMinFileSize)
                {
                    break;
                }
            } while (SleepOrQuit(timeoutInstant, pollSeconds));

            if (fileSize >= expectedMinFileSize)
            {
#endif
                m_Metadata.Resize(
                    fileSize,
                    "allocating metadata buffer, in call to BP5Reader Open");

                m_MDFileManager.ReadFile(m_Metadata.m_Buffer.data(), fileSize);
                m_MDFileAlreadyReadSize = fileSize;
                m_MDIndexFileAlreadyReadSize = metadataIndexFileSize;
                newIdxSize = metadataIndexFileSize;
#ifdef NOTDEF
            }
            else
            {
                throw std::ios_base::failure(
                    "ERROR: File " + m_Name +
                    " was found with an index file but md.0 "
                    "has not contained enough data within "
                    "the specified timeout of " +
                    std::to_string(timeoutSeconds.count()) +
                    " seconds. index size = " +
                    std::to_string(metadataIndexFileSize) +
                    " metadata size = " + std::to_string(fileSize) +
                    " expected size = " + std::to_string(expectedMinFileSize) +
                    ". One reason could be if the reader finds old data while "
                    "the writer is creating the new files.");
            }
#endif
        }
    }

    newIdxSize = m_Comm.BroadcastValue(newIdxSize, 0);

    if (newIdxSize > 0)
    {
        // broadcast buffer to all ranks from zero
        m_Comm.BroadcastVector(m_Metadata.m_Buffer);

        // broadcast metadata index buffer to all ranks from zero
        m_Comm.BroadcastVector(m_MetadataIndex.m_Buffer);

        // broadcast metadata index buffer to all ranks from zero
        m_Comm.BroadcastVector(m_MetaMetadata.m_Buffer);

        /* Parse metadata index table */
        ParseMetadataIndex(m_MetadataIndex, 0, true, false);
        // now we are sure the index header has been parsed, first step parsing
        // done

        m_BP5Deserializer = new format::BP5Deserializer(
            m_WriterCount, m_WriterIsRowMajor, m_ReaderIsRowMajor,
            (m_OpenMode == Mode::ReadRandomAccess));
        m_BP5Deserializer->m_Engine = this;

        InstallMetaMetaData(m_MetaMetadata);

        m_IdxHeaderParsed = true;

        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            for (size_t Step = 0; Step < m_MetadataIndexTable.size(); Step++)
            {
                InstallMetadataForTimestep(Step);
            }
        }
        // fills IO with Variables and Attributes
        //        m_MDFileProcessedSize = ParseMetadata(
        //            m_Metadata, *this, true);

        /* m_MDFileProcessedSize is the position in the buffer where processing
         * ends. The processing is controlled by the number of records in the
         * Index, which may be less than the actual entries in the metadata in a
         * streaming situation (where writer has just written metadata for step
         * K+1,...,K+L while the index contains K steps when the reader looks at
         * it).
         *
         * In ProcessMetadataForNewSteps(), we will re-read the metadata which
         * is in the buffer but has not been processed yet.
         */
    }
}

void BP5Reader::ParseMetadataIndex(format::BufferSTL &bufferSTL,
                                   const size_t absoluteStartPos,
                                   const bool hasHeader, const bool oneStepOnly)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t &position = bufferSTL.m_Position;

    if (hasHeader)
    {
        // Read header (64 bytes)
        // long version string
        position = m_VersionTagPosition;
        m_Minifooter.VersionTag.assign(&buffer[position], m_VersionTagLength);

        position = m_EndianFlagPosition;
        const uint8_t endianness = helper::ReadValue<uint8_t>(buffer, position);
        m_Minifooter.IsLittleEndian = (endianness == 0) ? true : false;
#ifndef ADIOS2_HAVE_ENDIAN_REVERSE
        if (helper::IsLittleEndian() != m_Minifooter.IsLittleEndian)
        {
            throw std::runtime_error(
                "ERROR: reader found BigEndian bp file, "
                "this version of ADIOS2 wasn't compiled "
                "with the cmake flag -DADIOS2_USE_Endian_Reverse=ON "
                "explicitly, in call to Open\n");
        }
#endif

        // This has no flag in BP5 header. Always true
        m_Minifooter.HasSubFiles = true;

        // BP version
        position = m_BPVersionPosition;
        m_Minifooter.Version = helper::ReadValue<uint8_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        if (m_Minifooter.Version != 5)
        {
            throw std::runtime_error(
                "ERROR: ADIOS2 BP5 Engine only supports bp format "
                "version 5, found " +
                std::to_string(m_Minifooter.Version) + " version \n");
        }

        // Writer active flag
        position = m_ActiveFlagPosition;
        const char activeChar = helper::ReadValue<uint8_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        m_WriterIsActive = (activeChar == '\1' ? true : false);
        position = m_WriterCountPosition;
        m_WriterCount = helper::ReadValue<uint32_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        position = m_AggregatorCountPosition;
        m_AggregatorCount = helper::ReadValue<uint32_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        position = m_ColumnMajorFlagPosition;
        const uint8_t val = helper::ReadValue<uint8_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        m_WriterIsRowMajor = val == 'n';
        // move position to first row
        position = 64;
    }

    for (uint64_t i = 0; i < m_WriterCount; i++)
    {
        m_WriterToFileMap.push_back(helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian));
    }

    // Read each record now
    uint64_t currentStep = 0;
    do
    {
        std::vector<uint64_t> ptrs;
        const uint64_t MetadataPos = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t MetadataSize = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t FlushCount = helper::ReadValue<uint64_t>(
            buffer, position, m_Minifooter.IsLittleEndian);

        ptrs.push_back(MetadataPos);
        ptrs.push_back(MetadataSize);
        ptrs.push_back(FlushCount);
        ptrs.push_back(position);
        m_MetadataIndexTable[currentStep] = ptrs;
#ifdef DUMPDATALOCINFO
        for (uint64_t i = 0; i < m_WriterCount; i++)
        {
            size_t DataPosPos = ptrs[3];
            std::cout << "Writer " << i << " data at ";
            for (uint64_t j = 0; j < FlushCount; j++)
            {
                const uint64_t DataPos = helper::ReadValue<uint64_t>(
                    buffer, DataPosPos, m_Minifooter.IsLittleEndian);
                const uint64_t DataSize = helper::ReadValue<uint64_t>(
                    buffer, DataPosPos, m_Minifooter.IsLittleEndian);
                std::cout << "loc:" << DataPos << " siz:" << DataSize << "; ";
            }
            const uint64_t DataPos = helper::ReadValue<uint64_t>(
                buffer, DataPosPos, m_Minifooter.IsLittleEndian);
            std::cout << "loc:" << DataPos << std::endl;
        }
#endif

        position += sizeof(uint64_t) * m_WriterCount * ((2 * FlushCount) + 1);
        m_StepsCount++;
        currentStep++;
    } while (!oneStepOnly && position < buffer.size());
}

#define declare_type(T)                                                        \
    void BP5Reader::DoGetSync(Variable<T> &variable, T *data)                  \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER("BP5Reader::Get");                              \
        GetSyncCommon(variable, data);                                         \
    }                                                                          \
    void BP5Reader::DoGetDeferred(Variable<T> &variable, T *data)              \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER("BP5Reader::Get");                              \
        GetDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void BP5Reader::DoClose(const int transportIndex)
{
    PERFSTUBS_SCOPED_TIMER("BP5Reader::Close");
    m_DataFileManager.CloseFiles();
    m_MDFileManager.CloseFiles();
}

// DoBlocksInfo will not be called because MinBlocksInfo is operative
#define declare_type(T)                                                        \
    std::vector<typename Variable<T>::BPInfo> BP5Reader::DoBlocksInfo(         \
        const Variable<T> &variable, const size_t step) const                  \
    {                                                                          \
        return std::vector<typename Variable<T>::BPInfo>();                    \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

size_t BP5Reader::DoSteps() const { return m_StepsCount; }

} // end namespace engine
} // end namespace core
} // end namespace adios2
