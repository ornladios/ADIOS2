/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DaosReader.cpp
 *
 */

#include "DaosReader.h"
#include "DaosReader.tcc"

#include "adios2/helper/adiosMath.h" // SetWithinLimit
#include <adios2-perfstubs-interface.h>

#include <chrono>
#include <errno.h>
#include <iomanip>
#include <mutex>
#include <stdbool.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

using TP = std::chrono::high_resolution_clock::time_point;
#define NOW() std::chrono::high_resolution_clock::now();
#define DURATION(T1, T2) static_cast<double>((T2 - T1).count()) / 1000000000.0;

#define DEBUG_BADALLOC
#undef DEBUG_BADALLOC

#define FAIL(fmt, ...)                                                                             \
    do                                                                                             \
    {                                                                                              \
        fprintf(stderr, "Process %d(%s): " fmt " aborting\n", m_Comm.Rank(), node, ##__VA_ARGS__); \
        exit(1);                                                                                   \
    } while (0)
#define ASSERT(cond, ...)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
            FAIL(__VA_ARGS__);                                                                     \
    } while (0)

namespace adios2
{
namespace core
{
namespace engine
{

DaosReader::DaosReader(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("DaosReader", io, name, mode, std::move(comm)), m_MDFileManager(io, m_Comm),
  m_DataFileManager(io, m_Comm), m_MDIndexFileManager(io, m_Comm),
  m_FileMetaMetadataManager(io, m_Comm), m_ActiveFlagFileManager(io, m_Comm)
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::Open");
    Init();
    m_IsOpen = true;
}

DaosReader::~DaosReader()
{
    if (m_BP5Deserializer)
        delete m_BP5Deserializer;
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

void DaosReader::DestructorClose(bool Verbose) noexcept
{
    // Nothing special needs to be done to "close" a BP5 reader during shutdown
    // if it hasn't already been Closed
    m_IsOpen = false;
}

void DaosReader::ReadMetadata(size_t Step)
{
    size_t pgstart = m_MetadataIndexTable[Step][0];
    const uint64_t WriterCount = m_WriterMap[m_WriterMapIndex[Step]].WriterCount;
    int rc;

    m_Metadata.Reset(true, false);

    // Reader rank 0 - reads all metadata
    if (m_Comm.Rank() == 0)
    {
        switch (daosEngine)
        {
        case DaosEngine::DAOS_ARRAY:
        case DaosEngine::DAOS_ARRAY_1MB_ALIGNED:
            DaosArrayReadMetadata(Step, WriterCount);
            break;
        case DaosEngine::DAOS_KV:
            DaosKVReadMetadata(Step, WriterCount);
            break;
        // Add other cases here if needed
        default:
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "ReadMetadata",
                                              "Unsupported DAOS interface");
        }
    }

    m_Comm.Barrier();

    // broadcast buffer to all ranks from zero
    CALI_MARK_BEGIN("DaosReader::broadcast_metadata");
    m_Comm.BroadcastVector(m_Metadata.m_Buffer);
    CALI_MARK_END("DaosReader::broadcast_metadata");
}

void DaosReader::DaosKVReadMetadata(size_t Step, uint64_t WriterCount)
{
    std::vector<size_t> list_writer_mdsize;
    list_writer_mdsize.reserve(WriterCount);
    char key[1000];
    int rc;
    size_t total_mdsize = 0;
    size_t buffer_size = 0;

    CALI_MARK_BEGIN("DaosReader::loop-getsize");

    // Async Get Size
    size_t WriterRank = 0;
    total_mdsize = 0;
    while (WriterRank < WriterCount)
    {
        int batchLimit = 0;

        // Start batch
        while (WriterRank < WriterCount && batchLimit < MAX_KV_GET_REQS)
        {
            sprintf(key, "step%zu-rank%zu", Step, WriterRank);
            CALI_MARK_BEGIN("DaosReader::daos_kv_get_size");
            rc = daos_kv_get(oh, DAOS_TX_NONE, 0, key, &list_writer_mdsize[WriterRank], NULL,
                             &ev[batchLimit]);
            ASSERT(rc == 0, "daos_kv_get() failed to read metadata with %d", rc);
            CALI_MARK_END("DaosReader::daos_kv_get_size");

            WriterRank++;
            batchLimit++;
        }

        int i = 0;
        while (1)
        {
            CALI_MARK_BEGIN("DaosReader::daos_eq_poll_getsize");
            rc = daos_eq_poll(eq, 1, DAOS_EQ_WAIT, batchLimit, evp);
            ASSERT(rc > 0, "daos_eq_poll() failed with %d", rc);
            CALI_MARK_END("DaosReader::daos_eq_poll_getsize");

            // If no events are processed, sleep for a short duration before polling again
            if (rc <= 0)
            {
                usleep(10000); // sleep for 10 milliseconds
                continue;
            }

            i += rc;
            if (i >= batchLimit)
                break;
        }
    }
    CALI_MARK_END("DaosReader::loop-getsize");

    for (int j = 0; j < WriterCount; j++)
        total_mdsize += list_writer_mdsize[j];

    // Reading total size of attribute includes vector of sizes of attributes and also the attribute
    // buffers
    size_t total_attr_size = 0;
    size_t off_attr = m_MetadataIndexTable[Step][4];
    m_MDFileManager.ReadFile((char *)&total_attr_size, sizeof(size_t), off_attr);
    off_attr = off_attr + sizeof(size_t);

    // std::cout << "Total attribute size: " << total_attr_size << std::endl;

    // Allocate memory for m_Metadata
    buffer_size = sizeof(uint64_t) * (WriterCount + 1) + total_mdsize + total_attr_size;
    m_Metadata.Resize(buffer_size, "allocating metadata buffer, in call to DaosReader Open");

    uint64_t *ptr = (uint64_t *)m_Metadata.m_Buffer.data();

    // The Metadata buffer is constructed like in WriteMetadata()
    ptr[0] = total_mdsize;
    int index = 1;
    for (WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        ptr[index] = list_writer_mdsize[WriterRank];
        index++;
    }
    m_MDFileManager.ReadFile((char *)&ptr[index], sizeof(uint64_t) * WriterCount, off_attr);
    off_attr = off_attr + sizeof(uint64_t) * WriterCount;

    /*
    for (WriterRank = 0; WriterRank < WriterCount; WriterRank++) {
       // std::cout << "Attributesize for writer " << WriterRank << " is " << ptr[index] <<
    std::endl; index++;
    }*/
    // Skip over the already read attribute sizes
    index += WriterCount;

    char *meta_buff = (char *)&ptr[index];
    index = 0;

    // Now read in the actual metadata for each writer
    WriterRank = 0;

    CALI_MARK_BEGIN("DaosReader::loop-get");
    while (WriterRank < WriterCount)
    {
        size_t ThisMDSize;
        int batchLimit = 0;

        // Start batch
        while (WriterRank < WriterCount && batchLimit < MAX_KV_GET_REQS)
        {
            ThisMDSize = list_writer_mdsize[WriterRank];

            sprintf(key, "step%zu-rank%zu", Step, WriterRank);
            CALI_MARK_BEGIN("DaosReader::daos_kv_get");
            rc = daos_kv_get(oh, DAOS_TX_NONE, 0, key, &ThisMDSize, &meta_buff[index],
                             &ev[batchLimit]);
            ASSERT(rc == 0, "daos_kv_get() failed to read metadata with %d", rc);
            CALI_MARK_END("DaosReader::daos_kv_get");

            index += ThisMDSize;

            WriterRank++;
            batchLimit++;
        }

        // Wait for all DaosKVOperations();
        int i = 0;
        while (1)
        {
            CALI_MARK_BEGIN("DaosReader::daos_eq_poll");
            rc = daos_eq_poll(eq, 1, DAOS_EQ_WAIT, batchLimit, evp);
            ASSERT(rc > 0, "daos_eq_poll() failed with %d", rc);
            CALI_MARK_END("DaosReader::daos_eq_poll");

            // If no events are processed, sleep for a short duration before polling again
            if (rc <= 0)
            {
                usleep(10000); // sleep for 10 milliseconds
                continue;
            }

            i += rc;
            if (i >= batchLimit)
                break;
        }

#ifdef DEBUG_BADALLOC
        // Print metadata block for the last writer in the batch
        printf("DaosReader::ReadMetadata MetadataBlock\n");
        char *tmp_ptr = &meta_buff[index - ThisMDSize];
        for (int i = 0; i < 20; i++)
            printf("%02x ", tmp_ptr[i]);
        printf("\n");
#endif
    }
    CALI_MARK_END("DaosReader::loop-get");

    // Read in attributes
    size_t att_readin_size = total_attr_size - (WriterCount * sizeof(uint64_t));
    m_MDFileManager.ReadFile((char *)&meta_buff[index], att_readin_size, off_attr);
}

void DaosReader::DaosArrayReadMetadata(size_t Step, uint64_t WriterCount)
{
    size_t total_mdsize = 0;
    size_t buffer_size = 0;
    size_t sizeof_list_writer_mdsize;
    uint64_t *list_writer_mdsize = (uint64_t *)malloc(sizeof(uint64_t) * WriterCount);
    daos_range_t *list_rg = NULL;

    // Get list of Metadata sizes for all writers
    char key[1000];

    if (daosEngine == DaosEngine::DAOS_ARRAY_1MB_ALIGNED)
    {
        list_rg = (daos_range_t *)malloc(WriterCount * sizeof(daos_range_t));
    }

    sprintf(key, "step%zu", Step);
    sizeof_list_writer_mdsize = sizeof(uint64_t) * WriterCount;

    CALI_MARK_BEGIN("DaosReader::daos_kv_get_list_of_mdsize");
    int rc = daos_kv_get(mdsize_oh, DAOS_TX_NONE, 0, key, &sizeof_list_writer_mdsize,
                         list_writer_mdsize, NULL);
    ASSERT(rc == 0, "daos_kv_get() failed to read metadata with %d", rc);
    CALI_MARK_END("DaosReader::daos_kv_get_list_of_mdsize");

    for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        total_mdsize += list_writer_mdsize[WriterRank];
    }

#ifdef DEBUG_BADALLOC
    std::cout << "ReadMetadata() - Step: " << Step << ", WriterCount: " << WriterCount
              << ", total_mdsize: " << total_mdsize << std::endl;
#endif

    // Reading total size of attribute includes vector of sizes of attributes and also the attribute
    // buffers
    size_t total_attr_size = 0;
    size_t off_attr = m_MetadataIndexTable[Step][4];
    m_MDFileManager.ReadFile((char *)&total_attr_size, sizeof(size_t), off_attr);
    off_attr = off_attr + sizeof(size_t);

    // Allocate memory for m_Metadata
    buffer_size = sizeof(uint64_t) * (WriterCount + 1) + total_mdsize + total_attr_size;
    m_Metadata.Resize(buffer_size, "allocating metadata buffer, "
                                   "in call to DaosReader Open");

    uint64_t *ptr = (uint64_t *)m_Metadata.m_Buffer.data();

    // The Metadata buffer is constructed like in WriteMetadata()
    ptr[0] = buffer_size;
    int index = 1;
    for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        ptr[index] = list_writer_mdsize[WriterRank];
        if (daosEngine == DaosEngine::DAOS_ARRAY_1MB_ALIGNED)
        {
            list_rg[WriterRank].rg_len = list_writer_mdsize[WriterRank];
            list_rg[WriterRank].rg_idx = m_step_offset + WriterRank * chunk_size_1mb;
        }
        index++;
    }

    m_MDFileManager.ReadFile((char *)&ptr[index], sizeof(uint64_t) * WriterCount, off_attr);
    off_attr = off_attr + sizeof(uint64_t) * WriterCount;

    index += WriterCount;

    char *meta_buff = (char *)&ptr[index];
    index = 0;

    // Now read in the actual metadata for each writer
    // Setup I/O Descriptor
    if (daosEngine == DaosEngine::DAOS_ARRAY_1MB_ALIGNED)
    {
        iod.arr_nr = WriterCount;
        iod.arr_rgs = list_rg;
    }
    else if (daosEngine == DaosEngine::DAOS_ARRAY)
    {
        iod.arr_nr = 1;
        rg.rg_len = total_mdsize;
        rg.rg_idx = m_step_offset;
        iod.arr_rgs = &rg;
    }

    /** set memory location */
    sgl.sg_nr = 1;
    d_iov_set(&iov, &meta_buff[index], total_mdsize);
    sgl.sg_iovs = &iov;

    // Write Metadata
    CALI_MARK_BEGIN("DaosReader::daos_array_read");
    rc = daos_array_read(oh, DAOS_TX_NONE, &iod, &sgl, NULL);
    ASSERT(rc == 0, "daos_array_read() failed to read metadata with %d", rc);
    CALI_MARK_END("DaosReader::daos_array_read");

    m_step_offset += MAX_AGGREGATE_METADATA_SIZE;
#ifdef DEBUG_BADALLOC
    size_t offset = 0;
    for (int j = 0; j < WriterCount; j++)
    {
        printf("ReadMetadata() Metadatablock, step = %lu, WriterRank = %d\n", Step, j);
        offset += list_writer_mdsize[j];
        for (int i = 0; i < 12; i++)
            printf("%02x ", meta_buff[offset + i]);
        printf("\n");
    }
#endif
    if (daosEngine == DaosEngine::DAOS_ARRAY_1MB_ALIGNED)
        free(list_rg);

    index += total_mdsize;

    // Read in attributes
    size_t att_readin_size = total_attr_size - (WriterCount * sizeof(uint64_t));
    m_MDFileManager.ReadFile((char *)&meta_buff[index], att_readin_size, off_attr);
    free(list_writer_mdsize);
}

void DaosReader::InstallMetadataForTimestep(size_t Step)
{
    // size_t pgstart = m_MetadataIndexTable[Step][0];
    size_t pgstart = m_MetadataIndexTable[0][0];
    size_t Position = pgstart + sizeof(uint64_t); // skip total data size
    const uint64_t WriterCount = m_WriterMap[m_WriterMapIndex[0]].WriterCount;
    // m_WriterMap[m_WriterMapIndex[Step]].WriterCount;
    size_t MDPosition = Position + 2 * sizeof(uint64_t) * WriterCount;
    for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        // variable metadata for timestep
        size_t ThisMDSize =
            helper::ReadValue<uint64_t>(m_Metadata.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        char *ThisMD = m_Metadata.m_Buffer.data() + MDPosition;
        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            m_BP5Deserializer->InstallMetaData(ThisMD, ThisMDSize, WriterRank, Step);
        }
        else
        {
            CALI_MARK_BEGIN("DaosReader::InstallMetaData");
            m_BP5Deserializer->InstallMetaData(ThisMD, ThisMDSize, WriterRank);
            CALI_MARK_END("DaosReader::InstallMetaData");
        }
        MDPosition += ThisMDSize;
    }

    for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        // attribute metadata for timestep
        size_t ThisADSize =
            helper::ReadValue<uint64_t>(m_Metadata.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        char *ThisAD = m_Metadata.m_Buffer.data() + MDPosition;
        if (ThisADSize > 0)
            m_BP5Deserializer->InstallAttributeData(ThisAD, ThisADSize);
        MDPosition += ThisADSize;
    }
}

StepStatus DaosReader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::BeginStep");

    // const char* fileName = "/home/hpcvenk1/benchmarks/wait-for-gdb.txt";
    // struct stat buffer;

    // if (m_Comm.Rank() == 0) {
    //     while (stat(fileName, &buffer) != 0) {
    //         sleep(1);  // Sleep for 1 second before checking again
    //     }
    //
    //     printf("File '%s' has been created!\n", fileName);
    // }
    // MPI_Barrier(MPI_COMM_WORLD);

    if (m_OpenMode == Mode::ReadRandomAccess)
    {
        helper::Throw<std::logic_error>("Engine", "DaosReader", "BeginStep",
                                        "BeginStep called in random access mode");
    }
    if (m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "DaosReader", "BeginStep",
                                        "BeginStep() is called a second time "
                                        "without an intervening EndStep()");
    }

    if (mode != StepMode::Read)
    {
        helper::Throw<std::invalid_argument>(
            "Engine", "DaosReader", "BeginStep",
            "mode is not supported yet, only Read is valid for engine "
            "DaosReader, in call to BeginStep");
    }

    StepStatus status = StepStatus::OK;
    if (m_FirstStep)
    {
        if (!m_StepsCount)
        {
            // not steps was found in Open/Init, check for new steps now
            status = CheckForNewSteps(Seconds(timeoutSeconds));
        }
    }
    else
    {
        if (m_CurrentStep + 1 >= m_StepsCount)
        {
            // we processed steps in memory, check for new steps now
            status = CheckForNewSteps(Seconds(timeoutSeconds));
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

        m_BP5Deserializer->SetupForStep(m_CurrentStep,
                                        m_WriterMap[m_WriterMapIndex[m_CurrentStep]].WriterCount);

        /* Remove all existing variables from previous steps
           It seems easier than trying to update them */
        // m_IO.RemoveAllVariables();
        CALI_MARK_BEGIN("DaosReader::metadata-acquisition");
        ReadMetadata(m_CurrentStep);
        CALI_MARK_END("DaosReader::metadata-acquisition");

        CALI_MARK_BEGIN("DaosReader::InstallMetadataForTimestep");
        InstallMetadataForTimestep(m_CurrentStep);
        CALI_MARK_END("DaosReader::InstallMetadataForTimestep");
        m_IO.ResetVariablesStepSelection(false, "in call to BP5 Reader BeginStep");

        // caches attributes for each step
        // if a variable name is a prefix
        // e.g. var  prefix = {var/v1, var/v2, var/v3}
        m_IO.SetPrefixedNames(true);
    }

    return status;
}

size_t DaosReader::CurrentStep() const { return m_CurrentStep; }

void DaosReader::EndStep()
{
    if (m_OpenMode == Mode::ReadRandomAccess)
    {
        helper::Throw<std::logic_error>("Engine", "DaosReader", "EndStep",
                                        "EndStep called in random access mode");
    }
    if (!m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "DaosReader", "EndStep",
                                        "EndStep() is called without a successful BeginStep()");
    }
    m_BetweenStepPairs = false;
    PERFSTUBS_SCOPED_TIMER("DaosReader::EndStep");

    if (m_DataFlag == DataFlag::ON)
    {
        CALI_MARK_BEGIN("DaosReader::PerformGets");
        PerformGets();
        CALI_MARK_END("DaosReader::PerformGets");
    }
    else
    {
        // Skip reading data
    }
}

std::pair<double, double> DaosReader::ReadData(adios2::transportman::TransportMan &FileManager,
                                               const size_t maxOpenFiles, const size_t WriterRank,
                                               const size_t Timestep, const size_t StartOffset,
                                               const size_t Length, char *Destination)
{
    /*
     * Warning: this function is called by multiple threads
     */
    size_t FlushCount = m_MetadataIndexTable[Timestep][2];
    size_t DataPosPos = m_MetadataIndexTable[Timestep][3];
    size_t SubfileNum =
        static_cast<size_t>(m_WriterMap[m_WriterMapIndex[Timestep]].RankToSubfile[WriterRank]);

    // check if subfile is already opened
    TP startSubfile = NOW();
    if (FileManager.m_Transports.count(SubfileNum) == 0)
    {
        const std::string subFileName =
            GetBPSubStreamName(m_Name, SubfileNum, m_Minifooter.HasSubFiles, true);
        if (FileManager.m_Transports.size() >= maxOpenFiles)
        {
            auto m = FileManager.m_Transports.begin();
            FileManager.CloseFiles((int)m->first);
        }
        FileManager.OpenFileID(subFileName, SubfileNum, Mode::Read, m_IO.m_TransportsParameters[0],
                               /*{{"transport", "File"}},*/ false);
    }
    TP endSubfile = NOW();
    double timeSubfile = DURATION(startSubfile, endSubfile);

    /* Each block is in exactly one flush. The StartOffset was calculated
       as if all the flushes were in a single contiguous block in file.
    */
    TP startRead = NOW();
    size_t InfoStartPos = DataPosPos + (WriterRank * (2 * FlushCount + 1) * sizeof(uint64_t));
    size_t SumDataSize = 0; // count in contiguous space
    for (size_t flush = 0; flush < FlushCount; flush++)
    {
        size_t ThisDataPos = helper::ReadValue<uint64_t>(m_MetadataIndex.m_Buffer, InfoStartPos,
                                                         m_Minifooter.IsLittleEndian);
        size_t ThisDataSize = helper::ReadValue<uint64_t>(m_MetadataIndex.m_Buffer, InfoStartPos,
                                                          m_Minifooter.IsLittleEndian);

        if (StartOffset < SumDataSize + ThisDataSize)
        {
            // discount offsets of skipped flushes
            size_t Offset = StartOffset - SumDataSize;
            FileManager.ReadFile(Destination, Length, ThisDataPos + Offset, SubfileNum);
            TP endRead = NOW();
            double timeRead = DURATION(startRead, endRead);
            return std::make_pair(timeSubfile, timeRead);
        }
        SumDataSize += ThisDataSize;
    }

    size_t ThisDataPos = helper::ReadValue<uint64_t>(m_MetadataIndex.m_Buffer, InfoStartPos,
                                                     m_Minifooter.IsLittleEndian);
    size_t Offset = StartOffset - SumDataSize;
    FileManager.ReadFile(Destination, Length, ThisDataPos + Offset, SubfileNum);

    TP endRead = NOW();
    double timeRead = DURATION(startRead, endRead);
    return std::make_pair(timeSubfile, timeRead);
}

void DaosReader::PerformGets()
{
    auto lf_CompareReqSubfile = [&](adios2::format::BP5Deserializer::ReadRequest &r1,
                                    adios2::format::BP5Deserializer::ReadRequest &r2) -> bool {
        return (m_WriterMap[m_WriterMapIndex[r1.Timestep]].RankToSubfile[r1.WriterRank] <
                m_WriterMap[m_WriterMapIndex[r2.Timestep]].RankToSubfile[r2.WriterRank]);
    };

    // TP start = NOW();
    PERFSTUBS_SCOPED_TIMER("DaosReader::PerformGets");
    size_t maxReadSize;

    // TP startGenerate = NOW();
    auto ReadRequests = m_BP5Deserializer->GenerateReadRequests(false, &maxReadSize);
    size_t nRequest = ReadRequests.size();
    // TP endGenerate = NOW();
    // double generateTime = DURATION(startGenerate, endGenerate);

    size_t nextRequest = 0;
    std::mutex mutexReadRequests;

    auto lf_GetNextRequest = [&]() -> size_t {
        std::lock_guard<std::mutex> lockGuard(mutexReadRequests);
        size_t reqidx = MaxSizeT;
        if (nextRequest < nRequest)
        {
            reqidx = nextRequest;
            ++nextRequest;
        }
        return reqidx;
    };

    auto lf_Reader = [&](const int FileManagerID,
                         const size_t maxOpenFiles) -> std::tuple<double, double, double, size_t> {
        double copyTotal = 0.0;
        double readTotal = 0.0;
        double subfileTotal = 0.0;
        size_t nReads = 0;
        std::vector<char> buf(maxReadSize);

        while (true)
        {
            const auto reqidx = lf_GetNextRequest();
            if (reqidx > nRequest)
            {
                break;
            }
            auto &Req = ReadRequests[reqidx];
            if (!Req.DestinationAddr)
            {
                Req.DestinationAddr = buf.data();
            }
            std::pair<double, double> t =
                ReadData(fileManagers[FileManagerID], maxOpenFiles, Req.WriterRank, Req.Timestep,
                         Req.StartOffset, Req.ReadLength, Req.DestinationAddr);

            TP startCopy = NOW();
            m_BP5Deserializer->FinalizeGet(Req, false);
            TP endCopy = NOW();
            subfileTotal += t.first;
            readTotal += t.second;
            copyTotal += DURATION(startCopy, endCopy);
            ++nReads;
        }
        return std::make_tuple(subfileTotal, readTotal, copyTotal, nReads);
    };

    // TP startRead = NOW();
    // double sortTime = 0.0;
    if (m_Threads > 1 && nRequest > 1)
    {
        // TP startSort = NOW();
        std::sort(ReadRequests.begin(), ReadRequests.end(), lf_CompareReqSubfile);
        // TP endSort = NOW();
        // sortTime = DURATION(startSort, endSort);
        size_t nThreads = (m_Threads < nRequest ? m_Threads : nRequest);

        size_t maxOpenFiles = helper::SetWithinLimit(
            (size_t)m_Parameters.MaxOpenFilesAtOnce / nThreads, (size_t)1, MaxSizeT);

        std::vector<std::future<std::tuple<double, double, double, size_t>>> futures(nThreads - 1);

        // launch Threads-1 threads to process subsets of requests,
        // then main thread process the last subset
        for (size_t tid = 0; tid < nThreads - 1; ++tid)
        {
            futures[tid] = std::async(std::launch::async, lf_Reader, tid + 1, maxOpenFiles);
        }
        // main thread runs last subset of reads
        /*auto tMain = */ lf_Reader(0, maxOpenFiles);
        /*{
            double tSubfile = std::get<0>(tMain);
            double tRead = std::get<1>(tMain);
            double tCopy = std::get<2>(tMain);
            size_t nReads = std::get<3>(tMain);
            std::cout << " -> PerformGets() thread MAIN total = "
                      << tSubfile + tRead + tCopy << "s, subfile = " << tSubfile
                      << "s, read = " << tRead << "s, copy = " << tCopy
                      << ", nReads = " << nReads << std::endl;
        }*/

        // wait for all async threads
        int tid = 1;
        for (auto &f : futures)
        {
            /*auto t = */ f.get();
            /*double tSubfile = std::get<0>(t);
            double tRead = std::get<1>(t);
            double tCopy = std::get<2>(t);
            size_t nReads = std::get<3>(t);
            std::cout << " -> PerformGets() thread " << tid
                      << " total = " << tSubfile + tRead + tCopy
                      << "s, subfile = " << tSubfile << "s, read = " << tRead
                      << "s, copy = " << tCopy << ", nReads = " << nReads
                      << std::endl;*/
            ++tid;
        }
    }
    else
    {
        size_t maxOpenFiles =
            helper::SetWithinLimit((size_t)m_Parameters.MaxOpenFilesAtOnce, (size_t)1, MaxSizeT);
        std::vector<char> buf(maxReadSize);
        for (auto &Req : ReadRequests)
        {
            if (!Req.DestinationAddr)
            {
                Req.DestinationAddr = buf.data();
            }
            ReadData(m_DataFileManager, maxOpenFiles, Req.WriterRank, Req.Timestep, Req.StartOffset,
                     Req.ReadLength, Req.DestinationAddr);
            m_BP5Deserializer->FinalizeGet(Req, false);
        }
    }

    // clear pending requests inside deserializer
    {
        std::vector<adios2::format::BP5Deserializer::ReadRequest> empty;
        m_BP5Deserializer->FinalizeGets(empty);
    }

    /*TP end = NOW();
    double t1 = DURATION(start, end);
    double t2 = DURATION(startRead, end);
    std::cout << " -> PerformGets() total = " << t1 << "s, Read loop = " << t2
              << "s, sort = " << sortTime << "s, generate = " << generateTime
              << ", nRequests = " << nRequest << std::endl;*/
}

// PRIVATE
void DaosReader::Init()
{
    if ((m_OpenMode != Mode::Read) && (m_OpenMode != Mode::ReadRandomAccess))
    {
        helper::Throw<std::invalid_argument>("Engine", "DaosReader", "Init",
                                             "DAOSReader only supports OpenMode::Read or "
                                             "OpenMode::ReadRandomAccess from" +
                                                 m_Name);
    }

    // if IO was involved in reading before this flag may be true now
    m_IO.m_ReadStreaming = false;
    m_ReaderIsRowMajor = (m_IO.m_ArrayOrder == ArrayOrdering::RowMajor);
    InitParameters();
    InitTransports();
    CALI_MARK_BEGIN("DaosReader::InitDAOS");
    InitDAOS();
    CALI_MARK_END("DaosReader::InitDAOS");
    if (!m_Parameters.SelectSteps.empty())
    {
        m_SelectedSteps.ParseSelection(m_Parameters.SelectSteps);
    }

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
    UpdateBuffer(timeoutInstant, pollSeconds / 10, timeoutSeconds);
}

void DaosReader::InitParameters()
{
    ParseParams(m_IO, m_Parameters);
    if (m_Parameters.OpenTimeoutSecs < 0.0f)
    {
        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            m_Parameters.OpenTimeoutSecs = 0.0f;
        }
        else
        {
            m_Parameters.OpenTimeoutSecs = 3600.0f;
        }
    }

    m_Threads = m_Parameters.Threads;
    if (m_Threads == 0)
    {
        helper::Comm m_NodeComm = m_Comm.GroupByShm("creating per-node comm at BP5 Open(read)");
        unsigned int NodeSize = static_cast<unsigned int>(m_NodeComm.Size());
        unsigned int NodeThreadSize = helper::NumHardwareThreadsPerNode();
        if (NodeThreadSize > 0)
        {
            m_Threads = helper::SetWithinLimit(NodeThreadSize / NodeSize, 1U, 16U);
        }
        else
        {
            m_Threads = helper::SetWithinLimit(8U / NodeSize, 1U, 8U);
        }
    }

    // Create m_Threads-1  extra file managers to be used by threads
    // The main thread uses the DataFileManager pushed here to vector[0]
    fileManagers.push_back(m_DataFileManager);
    for (unsigned int i = 0; i < m_Threads - 1; ++i)
    {
        fileManagers.push_back(
            transportman::TransportMan(transportman::TransportMan(m_IO, singleComm)));
    }

    size_t limit = helper::RaiseLimitNoFile();
    if (m_Parameters.MaxOpenFilesAtOnce > limit - 8)
    {
        m_Parameters.MaxOpenFilesAtOnce = limit - 8;
    }
}

bool DaosReader::SleepOrQuit(const TimePoint &timeoutInstant, const Seconds &pollSeconds)
{
    auto now = Now();
    if (now >= timeoutInstant)
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

size_t DaosReader::OpenWithTimeout(transportman::TransportMan &tm,
                                   const std::vector<std::string> &fileNames,
                                   const TimePoint &timeoutInstant, const Seconds &pollSeconds,
                                   std::string &lasterrmsg /*INOUT*/)
{
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    do
    {
        try
        {
            errno = 0;
            const bool profile = false; // m_BP4Deserializer.m_Profiler.m_IsActive;
            tm.OpenFiles(fileNames, adios2::Mode::Read, m_IO.m_TransportsParameters, profile);
            flag = 0; // found file
            break;
        }
        catch (std::ios_base::failure &e)
        {
            lasterrmsg = std::string("errno=" + std::to_string(errno) + ": " + e.what());
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

void DaosReader::OpenFiles(TimePoint &timeoutInstant, const Seconds &pollSeconds,
                           const Seconds &timeoutSeconds)
{
    /* Poll */
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    std::string lasterrmsg;
    if (m_Comm.Rank() == 0)
    {
        /* Open the metadata index table */
        const std::string metadataIndexFile(GetBPMetadataIndexFileName(m_Name));

        flag = OpenWithTimeout(m_MDIndexFileManager, {metadataIndexFile}, timeoutInstant,
                               pollSeconds, lasterrmsg);
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

            flag = OpenWithTimeout(m_MDFileManager, {metadataFile}, timeoutInstant, pollSeconds,
                                   lasterrmsg);
            if (flag != 0)
            {
                /* Close the metadata index table */
                m_MDIndexFileManager.CloseFiles();
            }
            else
            {
                /* Open the metametadata file */
                const std::string metametadataFile(GetBPMetaMetadataFileName(m_Name));

                /* We found md.idx. If we don't find md.0 immediately  we should
                 * wait a little bit hoping for the file system to catch up.
                 * This slows down finding the error in file reading mode but
                 * it will be more robust in streaming mode
                 */
                if (timeoutSeconds == Seconds(0.0))
                {
                    timeoutInstant += Seconds(5.0);
                }

                flag = OpenWithTimeout(m_FileMetaMetadataManager, {metametadataFile},
                                       timeoutInstant, pollSeconds, lasterrmsg);
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
            helper::Throw<std::ios_base::failure>("Engine", "DaosReader", "OpenFiles",
                                                  "File " + m_Name +
                                                      " cannot be opened: " + lasterrmsg);
        }
        else
        {
            helper::Throw<std::ios_base::failure>("Engine", "DaosReader", "OpenFiles",
                                                  "File " + m_Name + " cannot be opened");
        }
    }
    else if (flag == 1)
    {
        if (m_Comm.Rank() == 0)
        {
            helper::Throw<std::ios_base::failure>(
                "Engine", "DaosReader", "OpenFiles",
                "File " + m_Name + " could not be found within the " +
                    std::to_string(timeoutSeconds.count()) + "s timeout: " + lasterrmsg);
        }
        else
        {
            helper::Throw<std::ios_base::failure>(
                "Engine", "DaosReader", "OpenFiles",
                "File " + m_Name + " could not be found within the " +
                    std::to_string(timeoutSeconds.count()) + "s timeout");
        }
    }

    /* At this point we may have an empty index table.
     * The writer has created the file but no content may have been stored yet.
     */
}

MinVarInfo *DaosReader::MinBlocksInfo(const VariableBase &Var, const size_t Step) const
{
    return m_BP5Deserializer->MinBlocksInfo(Var, Step);
}

bool DaosReader::VarShape(const VariableBase &Var, const size_t Step, Dims &Shape) const
{
    return m_BP5Deserializer->VarShape(Var, Step, Shape);
}

bool DaosReader::VariableMinMax(const VariableBase &Var, const size_t Step, MinMaxStruct &MinMax)
{
    return m_BP5Deserializer->VariableMinMax(Var, Step, MinMax);
}

void DaosReader::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }
}

void DaosReader::array_oh_share(daos_handle_t *oh)
{
    d_iov_t ghdl = {NULL, 0, 0};
    int rc;

    if (m_Comm.Rank() == 0)
    {
        /** fetch size of global handle */
        rc = daos_array_local2global(*oh, &ghdl);
        ASSERT(rc == 0, "local2global failed with %d", rc);
    }

    /** broadcast size of global handle to all peers */
    m_Comm.Bcast((uint64_t *)&ghdl.iov_buf_len, 1, 0);

    /** allocate buffer for global pool handle */
    ghdl.iov_buf = malloc(ghdl.iov_buf_len);
    ghdl.iov_len = ghdl.iov_buf_len;

    if (m_Comm.Rank() == 0)
    {
        /** generate actual global handle to share with peer tasks */
        rc = daos_array_local2global(*oh, &ghdl);
        ASSERT(rc == 0, "local2global failed with %d", rc);
    }

    /** broadcast global handle to all peers */
    m_Comm.Bcast((char *)ghdl.iov_buf, ghdl.iov_len, 0);

    if (m_Comm.Rank() != 0)
    {
        /** unpack global handle */
        rc = daos_array_global2local(coh, ghdl, 0, oh);
        ASSERT(rc == 0, "global2local failed with %d", rc);
    }

    free(ghdl.iov_buf);

    m_Comm.Barrier();
}

void DaosReader::SetDataFlag()
{
    // Read environment variable DATA_STATE and set m_datastate accordingly
    const char *datastate = std::getenv("DATA_FLAG");
    if (!datastate)
    {
        // By default, set m_DataFlag to ON
        return;
    }

    // Set m_DataFlag based on the environment variable value
    m_DataFlag = (std::string(datastate) == "OFF") ? DataFlag::OFF : DataFlag::ON;
}

// Function to set DAOS interface from the environment variable
void DaosReader::SetDaosEngine()
{
    const char *env = std::getenv("DAOS_ENGINE");
    if (!env)
    {
        daosEngine = DaosEngine::UNKNOWN;
        return;
    }

    std::string interfaceStr(env);
    std::transform(interfaceStr.begin(), interfaceStr.end(), interfaceStr.begin(), ::tolower);
    if (interfaceStr == "daos-array")
    {
        daosEngine = DaosEngine::DAOS_ARRAY;
    }
    else if (interfaceStr == "daos-array-1mb-aligned")
    {
        daosEngine = DaosEngine::DAOS_ARRAY_1MB_ALIGNED;
    }
    else if (interfaceStr == "daos-kv")
    {
        daosEngine = DaosEngine::DAOS_KV;
    }
    else
    {
        daosEngine = DaosEngine::UNKNOWN;
    }
}

// Set m_PoolName and m_ContName from the environment variables DAOS_POOL and DAOS_CONT
void DaosReader::SetPoolAndContName()
{
    const char *pool = std::getenv("DAOS_POOL");
    const char *cont = std::getenv("DAOS_CONT");
    if (!pool || !cont)
    {
        std::cout << "DAOS_POOL or DAOS_CONT not set" << std::endl;
        exit(1);
    }

    strncpy(m_pool_label, pool, sizeof(m_pool_label) - 1);
    m_pool_label[sizeof(m_pool_label) - 1] = '\0';
    strncpy(m_cont_label, cont, sizeof(m_cont_label) - 1);
    m_cont_label[sizeof(m_cont_label) - 1] = '\0';
}

void DaosReader::ReadObjectIDsFromFile()
{
    std::string OIDFileName = GetOIDFileName(m_Name);
    FILE *fp = fopen(OIDFileName.c_str(), "r");
    if (fp == NULL)
    {
        perror("fopen");
        exit(1);
    }
    if (daosEngine == DaosEngine::DAOS_ARRAY || daosEngine == DaosEngine::DAOS_ARRAY_1MB_ALIGNED)
    {
        if (fscanf(fp, "%" SCNu64 "\n%" SCNu64 "\n", &oid.hi, &oid.lo) != 2)
        {
            fprintf(stderr, "Error reading OID from file\n");
            exit(1);
        }
        if (fscanf(fp, "%" SCNu64 "\n%" SCNu64 "\n", &mdsize_oid.hi, &mdsize_oid.lo) != 2)
        {
            fprintf(stderr, "Error reading OID from file\n");
            exit(1);
        }
    }
    else if (daosEngine == DaosEngine::DAOS_KV)
    {
        if (fscanf(fp, "%" SCNu64 "\n%" SCNu64 "\n", &oid.hi, &oid.lo) != 2)
        {
            fprintf(stderr, "Error reading OID from file\n");
            exit(1);
        }
    }
    fclose(fp);
}

void DaosReader::OpenDAOSObjects()
{
    int rc;
    if (daosEngine == DaosEngine::DAOS_ARRAY || daosEngine == DaosEngine::DAOS_ARRAY_1MB_ALIGNED)
    {
        daos_size_t cell_size = 1;
        daos_size_t chunk_size = 1048576;
        rc =
            daos_array_open(coh, oid, DAOS_TX_NONE, DAOS_OO_RO, &cell_size, &chunk_size, &oh, NULL);
        ASSERT(rc == 0, "daos_array_open failed with %d", rc);

        rc = daos_kv_open(coh, mdsize_oid, DAOS_OO_RO, &mdsize_oh, NULL);
        ASSERT(rc == 0, "daos_kv_open failed with %d", rc);
    }
    else if (daosEngine == DaosEngine::DAOS_KV)
    {
        // Open KV object
        CALI_MARK_BEGIN("DaosReader::daos_kv_open");
        rc = daos_kv_open(coh, oid, DAOS_OO_RO, &oh, NULL);
        ASSERT(rc == 0, "daos_kv_open failed with %d", rc);
        CALI_MARK_END("DaosReader::daos_kv_open");

        // Create event queue;
        CALI_MARK_BEGIN("DaosReader::EventQueueCreation");
        rc = daos_eq_create(&eq);
        CALI_MARK_END("DaosReader::EventQueueCreation");
        ASSERT(rc == 0, "daos_eq_create() failed with %d", rc);

        // Init events
        for (int i = 0; i < MAX_KV_GET_REQS; i++)
        {
            CALI_MARK_BEGIN("DaosReader::EventInitialization");
            rc = daos_event_init(&ev[i], eq, NULL);
            CALI_MARK_END("DaosReader::EventInitialization");
            ASSERT(rc == 0, "event init failed with %d", rc);
        }
    }
}

void DaosReader::InitDAOS()
{
    // Rank 0 - Connect to DAOS pool, and open container
    int rc;
    CALI_MARK_BEGIN("DaosReader::daos_init");
    rc = daos_init();
    ASSERT(rc == 0, "daos_init failed with %d", rc);
    CALI_MARK_END("DaosReader::daos_init");

    rc = gethostname(node, sizeof(node));
    ASSERT(rc == 0, "buffer for hostname too small");

    SetDaosEngine();
    SetPoolAndContName();
    SetDataFlag();

    CALI_MARK_BEGIN("DaosReader::daos_pool_connect");
    if (m_Comm.Rank() == 0)
    {
        /** connect to the just created DAOS pool */
        rc = daos_pool_connect(m_pool_label, DSS_PSETID,
                               // DAOS_PC_EX ,
                               DAOS_PC_RO /* read only access */, &poh /* returned pool handle */,
                               NULL /* returned pool info */, NULL /* event */);
        ASSERT(rc == 0, "pool connect failed with %d", rc);
    }

    CALI_MARK_END("DaosReader::daos_pool_connect");

    /** share pool handle with peer tasks */
    CALI_MARK_BEGIN("DaosReader::daos_handle_share_pool");
    if (m_Comm.Size() > 1)
        daos_handle_share(&poh, DaosReader::HANDLE_POOL);
    CALI_MARK_END("DaosReader::daos_handle_share_pool");

    CALI_MARK_BEGIN("DaosReader::daos_cont_open");

    if (m_Comm.Rank() == 0)
    {
        /** open container */
        rc = daos_cont_open(poh, m_cont_label, DAOS_COO_RO, &coh, NULL, NULL);
        ASSERT(rc == 0, "container open failed with %d", rc);
    }
    CALI_MARK_END("DaosReader::daos_cont_open");

    /** share container handle with peer tasks */
    CALI_MARK_BEGIN("DaosReader::daos_handle_share_cont");
    if (m_Comm.Size() > 1)
        daos_handle_share(&coh, HANDLE_CO);
    CALI_MARK_END("DaosReader::daos_handle_share_cont");

    CALI_MARK_BEGIN("DaosReader::fscanf-oid-n-broadcast");
    if (m_Comm.Rank() == 0)
    {
        ReadObjectIDsFromFile();
        OpenDAOSObjects();
    }
    CALI_MARK_END("DaosReader::fscanf-oid-n-broadcast");

    /*
      CALI_MARK_BEGIN("DaosReader::array_oh_share");
      if (m_Comm.Size() > 1)
      array_oh_share(&oh);
      CALI_MARK_END("DaosReader::array_oh_share");
    */
}

void DaosReader::InstallMetaMetaData(format::BufferSTL buffer)
{
    size_t Position = m_MetaMetaDataFileAlreadyProcessedSize;
    while (Position < buffer.m_Buffer.size())
    {
        format::BP5Base::MetaMetaInfoBlock MMI;

        MMI.MetaMetaIDLen =
            helper::ReadValue<uint64_t>(buffer.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        MMI.MetaMetaInfoLen =
            helper::ReadValue<uint64_t>(buffer.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        MMI.MetaMetaID = buffer.Data() + Position;
        MMI.MetaMetaInfo = buffer.Data() + Position + MMI.MetaMetaIDLen;
        m_BP5Deserializer->InstallMetaMetaData(MMI);
        Position += MMI.MetaMetaIDLen + MMI.MetaMetaInfoLen;
    }
    m_MetaMetaDataFileAlreadyProcessedSize = Position;
}

void DaosReader::UpdateBuffer(const TimePoint &timeoutInstant, const Seconds &pollSeconds,
                              const Seconds &timeoutSeconds)
{
    size_t newIdxSize = 0;
    m_MetadataIndex.Reset(true, false);
    if (m_Comm.Rank() == 0)
    {
        /* Read metadata index table into memory */
        const size_t metadataIndexFileSize = m_MDIndexFileManager.GetFileSize(0);
        newIdxSize = metadataIndexFileSize - m_MDIndexFileAlreadyReadSize;
        if (metadataIndexFileSize > m_MDIndexFileAlreadyReadSize)
        {
            m_MetadataIndex.m_Buffer.resize(newIdxSize);
            m_MDIndexFileManager.ReadFile(m_MetadataIndex.m_Buffer.data(), newIdxSize,
                                          m_MDIndexFileAlreadyReadSize);
        }
        else
        {
            m_MetadataIndex.m_Buffer.resize(0);
        }
    }

    // broadcast metadata index buffer to all ranks from zero
    m_Comm.BroadcastVector(m_MetadataIndex.m_Buffer);
    newIdxSize = m_MetadataIndex.m_Buffer.size();

    size_t parsedIdxSize = 0;
    const auto stepsBefore = m_StepsCount;
    if (newIdxSize > 0)
    {
        /* Parse metadata index table */
        const bool hasHeader = (!m_MDIndexFileAlreadyReadSize);
        parsedIdxSize = ParseMetadataIndex(m_MetadataIndex, 0, hasHeader);
        // now we are sure the index header has been parsed,
        // first step parsing done
        // m_FilteredMetadataInfo is created

        // cut down the index buffer by throwing away the read but unprocessed
        // steps
        m_MetadataIndex.m_Buffer.resize(parsedIdxSize);
        // next time read index file from this position
        m_MDIndexFileAlreadyReadSize += parsedIdxSize;

        // At this point first in time we learned the writer's major and we can
        // create the serializer object
        if (!m_BP5Deserializer)
        {
            m_BP5Deserializer = new format::BP5Deserializer(m_WriterIsRowMajor, m_ReaderIsRowMajor,
                                                            (m_OpenMode == Mode::ReadRandomAccess));
            m_BP5Deserializer->m_Engine = this;
        }
    }

    if (m_StepsCount > stepsBefore)
    {
        // m_Metadata.Reset(true, false);
        m_MetaMetadata.Reset(true, false);
        if (m_Comm.Rank() == 0)
        {
            //      // How much metadata do we need to read?
            //      size_t fileFilteredSize = 0;
            //      for (auto p : m_FilteredMetadataInfo) {
            //        fileFilteredSize += p.second;
            //      }
            //
            //      /* Read metadata file into memory but first make sure
            //       * it has the content that the index table refers to
            //       */
            //      auto p = m_FilteredMetadataInfo.back();
            //      uint64_t expectedMinFileSize = p.first + p.second;
            //      size_t actualFileSize = 0;
            //      do {
            //        actualFileSize = m_MDFileManager.GetFileSize(0);
            //        if (actualFileSize >= expectedMinFileSize) {
            //          break;
            //        }
            //      } while (SleepOrQuit(timeoutInstant, pollSeconds));
            //
            //      if (actualFileSize >= expectedMinFileSize) {
            //        m_Metadata.Resize(fileFilteredSize, "allocating metadata buffer, "
            //                                            "in call to DaosReader Open");
            //        size_t mempos = 0;
            //        for (auto p : m_FilteredMetadataInfo) {
            //          m_MDFileManager.ReadFile(m_Metadata.m_Buffer.data() + mempos,
            //                                   p.second, p.first);
            //          mempos += p.second;
            //        }
            //        m_MDFileAlreadyReadSize = expectedMinFileSize;
            //      } else {
            //                            helper::Throw<std::ios_base::failure>(
            //                                "Engine", "DaosReader", "UpdateBuffer",
            //                                "File " + m_Name +
            //                                    " was found with an index file but md.0 "
            //                                    "has not contained enough data within "
            //                                    "the specified timeout of " +
            //                                    std::to_string(timeoutSeconds.count()) +
            //                                    " seconds. index size = " +
            //                                    std::to_string(newIdxSize) + " metadata
            //                                    size = " + std::to_string(actualFileSize)
            //                                    + " expected size = " +
            //                                    std::to_string(expectedMinFileSize) +
            //                                    ". One reason could be if the reader finds
            //                                    old " "data " "while " "the writer is
            //                                    creating the new files.");
            //      }

            /* Read new meta-meta-data into memory and append to existing one in
             * memory */
            const size_t metametadataFileSize = m_FileMetaMetadataManager.GetFileSize(0);
            if (metametadataFileSize > m_MetaMetaDataFileAlreadyReadSize)
            {
                const size_t newMMDSize = metametadataFileSize - m_MetaMetaDataFileAlreadyReadSize;
                m_MetaMetadata.Resize(metametadataFileSize, "(re)allocating meta-meta-data buffer, "
                                                            "in call to DaosReader Open");
                m_FileMetaMetadataManager.ReadFile(m_MetaMetadata.m_Buffer.data() +
                                                       m_MetaMetaDataFileAlreadyReadSize,
                                                   newMMDSize, m_MetaMetaDataFileAlreadyReadSize);
                m_MetaMetaDataFileAlreadyReadSize += newMMDSize;
            }
        }

        // broadcast buffer to all ranks from zero
        // m_Comm.BroadcastVector(m_Metadata.m_Buffer);

        // broadcast metadata index buffer to all ranks from zero
        m_Comm.BroadcastVector(m_MetaMetadata.m_Buffer);

        InstallMetaMetaData(m_MetaMetadata);

        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            for (size_t Step = 0; Step < m_MetadataIndexTable.size(); Step++)
            {
                m_BP5Deserializer->SetupForStep(Step,
                                                m_WriterMap[m_WriterMapIndex[Step]].WriterCount);
                InstallMetadataForTimestep(Step);
            }
        }
    }
}

size_t DaosReader::ParseMetadataIndex(format::BufferSTL &bufferSTL, const size_t absoluteStartPos,
                                      const bool hasHeader)
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
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "ParseMetadataIndex",
                                              "reader found BigEndian bp file, "
                                              "this version of ADIOS2 wasn't compiled "
                                              "with the cmake flag -DADIOS2_USE_Endian_Reverse=ON "
                                              "explicitly, in call to Open");
        }
#endif

        // This has no flag in BP5 header. Always true
        m_Minifooter.HasSubFiles = true;

        // BP version
        position = m_BPVersionPosition;
        m_Minifooter.Version =
            helper::ReadValue<uint8_t>(buffer, position, m_Minifooter.IsLittleEndian);
        if (m_Minifooter.Version != 5)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "ParseMetadataIndex",
                                              "ADIOS2 BP5 Engine only supports bp format "
                                              "version 5, found " +
                                                  std::to_string(m_Minifooter.Version) +
                                                  " version");
        }

        // BP minor version, unused
        position = m_BPMinorVersionPosition;
        const uint8_t minorversion =
            helper::ReadValue<uint8_t>(buffer, position, m_Minifooter.IsLittleEndian);
        if (minorversion != m_BP5MinorVersion)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "ParseMetadataIndex",
                                              "Current ADIOS2 BP5 Engine only supports version 5." +
                                                  std::to_string(m_BP5MinorVersion) + ", found 5." +
                                                  std::to_string(minorversion) + " version");
        }

        // Writer active flag
        position = m_ActiveFlagPosition;
        const char activeChar =
            helper::ReadValue<uint8_t>(buffer, position, m_Minifooter.IsLittleEndian);
        m_WriterIsActive = (activeChar == '\1' ? true : false);

        position = m_ColumnMajorFlagPosition;
        const uint8_t val =
            helper::ReadValue<uint8_t>(buffer, position, m_Minifooter.IsLittleEndian);
        m_WriterIsRowMajor = val == 'n';
        // move position to first row
        position = m_IndexHeaderSize;
    }

    // set a limit for metadata size in streaming mode
    size_t maxMetadataSizeInMemory = adios2::MaxSizeT;
    if (m_OpenMode == Mode::Read)
    {
        maxMetadataSizeInMemory = 16777216; // 16MB
    }
    size_t metadataSizeToRead = 0;

    // Read each record now
    uint64_t MetadataPosTotalSkip = 0;
    m_MetadataIndexTable.clear();
    m_FilteredMetadataInfo.clear();
    uint64_t minfo_pos = 0;
    uint64_t minfo_size = 0;
    int n = 0;    // a loop counter for current run4
    int nrec = 0; // number of records in current run

    while (position < buffer.size())
    {

        const unsigned char recordID =
            helper::ReadValue<unsigned char>(buffer, position, m_Minifooter.IsLittleEndian);
        const uint64_t recordLength =
            helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
        const size_t dbgRecordStartPosition = position;

        switch (recordID)
        {
        case IndexRecord::WriterMapRecord: {
            auto p = m_WriterMap.emplace(m_StepsCount, WriterMapStruct());
            auto &s = p.first->second;
            s.WriterCount =
                helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
            s.AggregatorCount =
                helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
            s.SubfileCount =
                helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
            // Get the process -> subfile map
            s.RankToSubfile.reserve(s.WriterCount);
            for (uint64_t i = 0; i < s.WriterCount; i++)
            {
                const uint64_t subfileIdx =
                    helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
                s.RankToSubfile.push_back(subfileIdx);
            }
            m_LastMapStep = m_StepsCount;
            m_LastWriterCount = s.WriterCount;
            break;
        }
        case IndexRecord::StepRecord: {
            std::vector<uint64_t> ptrs;
            const uint64_t MetadataPos =
                helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
            const uint64_t MetadataSize =
                helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);
            const uint64_t FlushCount =
                helper::ReadValue<uint64_t>(buffer, position, m_Minifooter.IsLittleEndian);

            if (!n)
            {
                minfo_pos = MetadataPos; // initialize minfo_pos properly
                MetadataPosTotalSkip = MetadataPos;
            }

            if (m_SelectedSteps.IsSelected(m_AbsStepsInFile))
            {
                m_WriterMapIndex.push_back(m_LastMapStep);

                // pos in metadata in memory
                ptrs.push_back(MetadataPos - MetadataPosTotalSkip);
                ptrs.push_back(MetadataSize);
                ptrs.push_back(FlushCount);
                ptrs.push_back(position);
                // absolute pos in file before read
                ptrs.push_back(MetadataPos);
                m_MetadataIndexTable[m_StepsCount] = ptrs;
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
                minfo_size += MetadataSize;
                metadataSizeToRead += MetadataSize;
                m_StepsCount++;
            }
            else
            {
                MetadataPosTotalSkip += MetadataSize;
                if (minfo_size > 0)
                {
                    m_FilteredMetadataInfo.push_back(std::make_pair(minfo_pos, minfo_size));
                }
                minfo_pos = MetadataPos + MetadataSize;
                minfo_size = 0;
            }

            // skip over the writer -> data file offset records
            position += sizeof(uint64_t) * m_LastWriterCount * ((2 * FlushCount) + 1);
            ++m_AbsStepsInFile;
            ++n;
            break;
        }
        }
        // dbg
        if ((position - dbgRecordStartPosition) != (size_t)recordLength)
        {
            helper::Throw<std::runtime_error>(
                "Engine", "DaosReader", "ParseMetadataIndex",
                "Record " + std::to_string(nrec) + " (id = " + std::to_string(recordID) +
                    ") has invalid length " + std::to_string(recordLength) + ". We parsed " +
                    std::to_string(position - dbgRecordStartPosition) + " bytes for this record");
        }
        ++nrec;
    }
    if (minfo_size > 0)
    {
        m_FilteredMetadataInfo.push_back(std::make_pair(minfo_pos, minfo_size));
    }

    return position;
}

bool DaosReader::ReadActiveFlag(std::vector<char> &buffer)
{
    if (buffer.size() < m_ActiveFlagPosition)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "ReadActiveFlag",
                                          "called with a buffer smaller than required");
    }
    // Writer active flag
    size_t position = m_ActiveFlagPosition;
    const char activeChar =
        helper::ReadValue<uint8_t>(buffer, position, m_Minifooter.IsLittleEndian);
    m_WriterIsActive = (activeChar == '\1' ? true : false);
    return m_WriterIsActive;
}

bool DaosReader::CheckWriterActive()
{
    size_t flag = 1;
    if (m_Comm.Rank() == 0)
    {
        auto fsize = m_MDIndexFileManager.GetFileSize(0);
        if (fsize >= m_IndexHeaderSize)
        {
            std::vector<char> header(m_IndexHeaderSize, '\0');
            m_MDIndexFileManager.ReadFile(header.data(), m_IndexHeaderSize, 0, 0);
            bool active = ReadActiveFlag(header);
            flag = (active ? 1 : 0);
        }
    }
    flag = m_Comm.BroadcastValue(flag, 0);
    m_WriterIsActive = (flag > 0);
    return m_WriterIsActive;
}

StepStatus DaosReader::CheckForNewSteps(Seconds timeoutSeconds)
{
    /* Do a collective wait for a step within timeout.
       Make sure every reader comes to the same conclusion */
    StepStatus retval = StepStatus::OK;

    if (timeoutSeconds < Seconds::zero())
    {
        timeoutSeconds = Seconds(999999999); // max 1 billion seconds wait
    }
    const TimePoint timeoutInstant = Now() + timeoutSeconds;

    auto pollSeconds = Seconds(m_Parameters.BeginStepPollingFrequencySecs);
    if (pollSeconds > timeoutSeconds)
    {
        pollSeconds = timeoutSeconds;
    }

    /* Poll */
    const auto stepsBefore = m_StepsCount;
    do
    {
        UpdateBuffer(timeoutInstant, pollSeconds / 10, timeoutSeconds);
        if (m_StepsCount > stepsBefore)
        {
            break;
        }
        if (!CheckWriterActive())
        {
            /* Race condition: When checking data in UpdateBuffer, new
             * step(s) may have not arrived yet. When checking active flag,
             * the writer may have completed write and terminated. So we may
             * have missed a step or two. */
            UpdateBuffer(timeoutInstant, pollSeconds / 10, timeoutSeconds);
            break;
        }
    } while (SleepOrQuit(timeoutInstant, pollSeconds));

    if (m_StepsCount > stepsBefore)
    {
        /* we have got new steps and new metadata in memory */
        retval = StepStatus::OK;
    }
    else
    {
        m_IO.m_ReadStreaming = false;
        if (m_WriterIsActive)
        {
            retval = StepStatus::NotReady;
        }
        else
        {
            retval = StepStatus::EndOfStream;
        }
    }
    return retval;
}

void DaosReader::DoGetAbsoluteSteps(const VariableBase &variable, std::vector<size_t> &keys) const
{
    m_BP5Deserializer->GetAbsoluteSteps(variable, keys);
    return;
}

#define declare_type(T)                                                                            \
    void DaosReader::DoGetSync(Variable<T> &variable, T *data)                                     \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("DaosReader::Get");                                                 \
        GetSyncCommon(variable, data);                                                             \
    }                                                                                              \
    void DaosReader::DoGetDeferred(Variable<T> &variable, T *data)                                 \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("DaosReader::Get");                                                 \
        GetDeferredCommon(variable, data);                                                         \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void DaosReader::DoGetStructSync(VariableStruct &variable, void *data)
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::Get");
    GetSyncCommon(variable, data);
}

void DaosReader::DoGetStructDeferred(VariableStruct &variable, void *data)
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::Get");
    GetDeferredCommon(variable, data);
}

void DaosReader::DoClose(const int transportIndex)
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::Close");
    if (m_OpenMode == Mode::ReadRandomAccess)
    {
        PerformGets();
    }
    else if (m_BetweenStepPairs)
    {
        EndStep();
    }
    m_DataFileManager.CloseFiles();
    m_MDFileManager.CloseFiles();
    m_MDIndexFileManager.CloseFiles();
    m_FileMetaMetadataManager.CloseFiles();
    for (unsigned int i = 1; i < m_Threads; ++i)
    {
        fileManagers[i].CloseFiles();
    }
}

size_t DaosReader::DoSteps() const { return m_StepsCount; }

void DaosReader::NotifyEngineNoVarsQuery()
{
    if (!m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>(
            "Engine", "DaosReader", "NotifyEngineNoVarsQuery",
            "You've called InquireVariable() when the IO is empty and "
            "outside a BeginStep/EndStep pair.  If this is code that is "
            "newly "
            "transititioning to the BP5 file engine, you may be relying "
            "upon "
            "deprecated behaviour.  If you intend to use ADIOS using the "
            "Begin/EndStep interface, move all InquireVariable calls "
            "inside "
            "the BeginStep/EndStep pair.  If intending to use "
            "random-access "
            "file mode, change your Open() mode parameter to "
            "Mode::ReadRandomAccess.");
    }
}

void DaosReader::daos_handle_share(daos_handle_t *hdl, int type)
{
    d_iov_t ghdl = {NULL, 0, 0};
    int rc;

    CALI_MARK_BEGIN("DaosReader::local2global+broadcast_sizeofhandle");
    if (m_Comm.Rank() == 0)
    {
        /** fetch size of global handle */
        if (type == DaosReader::HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else
            rc = daos_cont_local2global(*hdl, &ghdl);
        ASSERT(rc == 0, "local2global failed with %d", rc);
    }

    /** broadcast size of global handle to all peers */
    m_Comm.Bcast((uint64_t *)&ghdl.iov_buf_len, 1, 0);
    CALI_MARK_END("DaosReader::local2global+broadcast_sizeofhandle");

    /** allocate buffer for global pool handle */
    ghdl.iov_buf = malloc(ghdl.iov_buf_len);
    ghdl.iov_len = ghdl.iov_buf_len;

    CALI_MARK_BEGIN("DaosReader::local2global+broadcast_handle");
    if (m_Comm.Rank() == 0)
    {
        /** generate actual global handle to share with peer tasks */
        if (type == DaosReader::HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else
            rc = daos_cont_local2global(*hdl, &ghdl);
        ASSERT(rc == 0, "local2global failed with %d", rc);
    }

    /** broadcast global handle to all peers */
    m_Comm.Bcast((char *)ghdl.iov_buf, ghdl.iov_len, 0);
    CALI_MARK_END("DaosReader::local2global+broadcast_handle");

    CALI_MARK_BEGIN("DaosReader::global2local+barrier");
    if (m_Comm.Rank() != 0)
    {
        /** unpack global handle */
        if (type == DaosReader::HANDLE_POOL)
        {
            /* NB: Only pool_global2local are different */
            rc = daos_pool_global2local(ghdl, hdl);
        }
        else
        {
            rc = daos_cont_global2local(poh, ghdl, hdl);
        }
        ASSERT(rc == 0, "global2local failed with %d", rc);
    }

    free(ghdl.iov_buf);

    m_Comm.Barrier();
    CALI_MARK_END("DaosReader::global2local+barrier");
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
