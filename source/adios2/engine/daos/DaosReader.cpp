/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DaosReader.h"
#include "DaosReader.tcc"

#include "adios2/helper/adiosMath.h" // SetWithinLimit
#include "adios2/toolkit/transport/OpenFile.h"
#include <adios2-perfstubs-interface.h>

#include <algorithm> // std::min
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef> // ptrdiff_t
#include <cstdio>  // fprintf (DAOS_MD_DEBUG)
#include <cstdlib> // getenv
#include <cstring> // memcpy
#include <deque>
#include <errno.h>
#include <exception> // std::exception (DAOS_MD_DEBUG)
#include <fstream>
#include <iomanip>
#include <limits> // numeric_limits (DAOS_SUBRANGE_BATCH=0)
#include <memory> // unique_ptr (uninitialized read scratch)
#include <mutex>
#include <sstream>
#include <stdbool.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

#include <daos_array.h>

using TP = std::chrono::high_resolution_clock::time_point;
#define NOW() std::chrono::high_resolution_clock::now();
#define DURATION(T1, T2) static_cast<double>((T2 - T1).count()) / 1000000000.0;

namespace adios2
{
namespace core
{
namespace engine
{

DaosReader::DaosReader(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("DaosReader", io, name, mode, std::move(comm))
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
        switch (metadataLayout)
        {
        case MetadataLayout::ARRAY:
        case MetadataLayout::ARRAY_1MB_ALIGNED:
            DaosArrayReadMetadata(Step, WriterCount);
            break;
        case MetadataLayout::KV:
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
            sprintf(attrkey[batchLimit], "step%zu-rank%zu", Step, WriterRank);
            CALI_MARK_BEGIN("DaosReader::daos_kv_get_size");
            rc = daos_kv_get(oh, DAOS_TX_NONE, 0, attrkey[batchLimit],
                             &list_writer_mdsize[WriterRank], NULL, &ev[batchLimit]);
            if (rc)
            {
                helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosKVReadMetadata",
                                                  "daos_kv_get (size) failed rc=" +
                                                      std::to_string(rc));
            }
            CALI_MARK_END("DaosReader::daos_kv_get_size");

            WriterRank++;
            batchLimit++;
        }

        int i = 0;
        while (1)
        {
            CALI_MARK_BEGIN("DaosReader::daos_eq_poll_getsize");
            rc = daos_eq_poll(eq, 1, DAOS_EQ_WAIT, batchLimit, evp);
            if (rc < 0)
            {
                helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosKVReadMetadata",
                                                  "daos_eq_poll (size) failed rc=" +
                                                      std::to_string(rc));
            }
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

    // In per-rank mode the attributes ride in each rank's blob (no md.0
    // aggregate writes), so skip the md.0 reads and the attribute-size
    // layer in the buffer.
    size_t total_attr_size = 0;
    size_t off_attr = 0;
    if (!m_PerRankMetadata)
    {
        off_attr = m_MetadataIndexTable[Step][2];
        m_MetadataFile->Read((char *)&total_attr_size, sizeof(size_t), off_attr);
        off_attr = off_attr + sizeof(size_t);
    }

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
    if (!m_PerRankMetadata)
    {
        m_MetadataFile->Read((char *)&ptr[index], sizeof(uint64_t) * WriterCount, off_attr);
        off_attr = off_attr + sizeof(uint64_t) * WriterCount;
        // Skip over the already read attribute sizes
        index += WriterCount;
    }

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

            sprintf(attrkey[batchLimit], "step%zu-rank%zu", Step, WriterRank);
            CALI_MARK_BEGIN("DaosReader::daos_kv_get");
            rc = daos_kv_get(oh, DAOS_TX_NONE, 0, attrkey[batchLimit],
                             &list_writer_mdsize[WriterRank], &meta_buff[index], &ev[batchLimit]);
            if (rc)
            {
                helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosKVReadMetadata",
                                                  "daos_kv_get failed rc=" + std::to_string(rc));
            }
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
            if (rc < 0)
            {
                helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosKVReadMetadata",
                                                  "daos_eq_poll failed rc=" + std::to_string(rc));
            }
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
    }
    CALI_MARK_END("DaosReader::loop-get");

    if (!m_PerRankMetadata)
    {
        // Read in attributes (aggregated mode only — per-rank carries attrs
        // inline in the per-rank blob).
        size_t att_readin_size = total_attr_size - (WriterCount * sizeof(uint64_t));
        m_MetadataFile->Read((char *)&meta_buff[index], att_readin_size, off_attr);
    }
}

void DaosReader::DaosArrayReadMetadata(size_t Step, uint64_t WriterCount)
{
    size_t total_mdsize = 0;
    size_t buffer_size = 0;
    size_t sizeof_list_writer_mdsize;

    // Zero writers would reach daos_array_read below as arr_nr=0 with a NULL
    // arr_rgs, and malloc(0) is free to return NULL, so reject it up front
    // rather than let it become an allocation check we cannot make.
    if (WriterCount == 0)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosArrayReadMetadata",
                                          "step " + std::to_string(Step) + " reports zero writers");
    }

    uint64_t *list_writer_mdsize = (uint64_t *)malloc(sizeof(uint64_t) * WriterCount);
    if (list_writer_mdsize == NULL)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosArrayReadMetadata",
                                          "out of memory for the mdsize list of " +
                                              std::to_string(WriterCount) + " writers");
    }
    daos_range_t *list_rg = NULL;

    // Get list of Metadata sizes for all writers
    char key[1000];

    if (metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {
        // This buffer becomes iod.arr_rgs while arr_nr is set from WriterCount
        // independently, so a silent NULL here is a malformed IOD (nr>0 with no
        // ranges) handed to DAOS under exactly the memory pressure that caused it.
        list_rg = (daos_range_t *)malloc(WriterCount * sizeof(daos_range_t));
        if (list_rg == NULL)
        {
            free(list_writer_mdsize);
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosArrayReadMetadata",
                                              "out of memory for the range list of " +
                                                  std::to_string(WriterCount) + " writers");
        }
    }

    sprintf(key, "step%zu", Step);
    sizeof_list_writer_mdsize = sizeof(uint64_t) * WriterCount;

    CALI_MARK_BEGIN("DaosReader::daos_kv_get_list_of_mdsize");
    int rc = daos_kv_get(mdsize_oh, DAOS_TX_NONE, 0, key, &sizeof_list_writer_mdsize,
                         list_writer_mdsize, NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosArrayReadMetadata",
                                          "daos_kv_get (mdsize list) failed rc=" +
                                              std::to_string(rc));
    }
    CALI_MARK_END("DaosReader::daos_kv_get_list_of_mdsize");

    for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        total_mdsize += list_writer_mdsize[WriterRank];
    }

    // In per-rank mode attributes ride in each rank's blob — skip md.0.
    size_t total_attr_size = 0;
    size_t off_attr = 0;
    if (!m_PerRankMetadata)
    {
        off_attr = m_MetadataIndexTable[Step][2];
        m_MetadataFile->Read((char *)&total_attr_size, sizeof(size_t), off_attr);
        off_attr = off_attr + sizeof(size_t);
    }

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
        if (metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
        {
            list_rg[WriterRank].rg_len = list_writer_mdsize[WriterRank];
            list_rg[WriterRank].rg_idx = m_step_offset + WriterRank * chunk_size_1mb;
        }
        index++;
    }

    if (!m_PerRankMetadata)
    {
        m_MetadataFile->Read((char *)&ptr[index], sizeof(uint64_t) * WriterCount, off_attr);
        off_attr = off_attr + sizeof(uint64_t) * WriterCount;
        index += WriterCount;
    }

    char *meta_buff = (char *)&ptr[index];
    index = 0;

    // Now read in the actual metadata for each writer
    // Setup I/O Descriptor
    if (metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {
        iod.arr_nr = WriterCount;
        iod.arr_rgs = list_rg;
    }
    else if (metadataLayout == MetadataLayout::ARRAY)
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
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "DaosArrayReadMetadata",
                                          "daos_array_read (metadata) failed rc=" +
                                              std::to_string(rc));
    }
    CALI_MARK_END("DaosReader::daos_array_read");

    if (std::getenv("DAOS_MD_DEBUG") && Step == 0)
    {
        const unsigned char *mb = (const unsigned char *)meta_buff;
        fprintf(stderr,
                "[DAOS_MD_DEBUG] ArrayReadMD step=%zu layout=%d WriterCount=%zu "
                "total_mdsize=%zu total_attr_size=%zu buffer_size=%zu m_step_offset=%zu\n",
                Step, (int)metadataLayout, (size_t)WriterCount, total_mdsize, total_attr_size,
                buffer_size, m_step_offset);
        // Print a spread of blobs so the *shape* of any buffer corruption is
        // visible right after the array read: which ranks carry a valid FFS
        // id (first bytes 02 00 00 92 for EFFIS-XG) and which are garbage.
        const size_t probes[] = {0,  1,   2,   3,   4,    5,    8,    16,  32,
                                 64, 128, 256, 512, 1024, 2048, 3072, 4000};
        for (size_t pi = 0; pi < sizeof(probes) / sizeof(probes[0]); ++pi)
        {
            size_t w = probes[pi];
            if (w >= WriterCount)
                continue;
            size_t cum = 0;
            for (size_t j = 0; j < w; ++j)
                cum += list_writer_mdsize[j];
            const unsigned char *bp = mb + cum;
            fprintf(stderr,
                    "[DAOS_MD_DEBUG]   blob[%zu] mdsize=%llu off=%zu first=%02x%02x%02x%02x\n", w,
                    (unsigned long long)list_writer_mdsize[w], cum, bp[0], bp[1], bp[2], bp[3]);
        }
        size_t lastw = WriterCount - 1, lastcum = 0;
        for (size_t j = 0; j < lastw; ++j)
            lastcum += list_writer_mdsize[j];
        const unsigned char *lp = mb + lastcum;
        fprintf(stderr,
                "[DAOS_MD_DEBUG]   blob[%zu](last) mdsize=%llu off=%zu first=%02x%02x%02x%02x\n",
                lastw, (unsigned long long)list_writer_mdsize[lastw], lastcum, lp[0], lp[1], lp[2],
                lp[3]);
    }

    m_step_offset += MAX_AGGREGATE_METADATA_SIZE;
    if (metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
        free(list_rg);

    index += total_mdsize;

    if (!m_PerRankMetadata)
    {
        // Read in attributes (aggregated mode only).
        size_t att_readin_size = total_attr_size - (WriterCount * sizeof(uint64_t));
        m_MetadataFile->Read((char *)&meta_buff[index], att_readin_size, off_attr);
    }
    free(list_writer_mdsize);
}

void DaosReader::InstallMetadataForTimestep(size_t Step)
{
    // size_t pgstart = m_MetadataIndexTable[Step][0];
    size_t pgstart = m_MetadataIndexTable[0][0];
    size_t Position = pgstart + sizeof(uint64_t); // skip total data size
    const uint64_t WriterCount = m_WriterMap[m_WriterMapIndex[0]].WriterCount;
    auto &stepStarts = m_RankStartDataPos[Step];
    stepStarts.resize(WriterCount);

    const bool mdDebug = std::getenv("DAOS_MD_DEBUG") != nullptr;
    if (mdDebug)
        fprintf(stderr,
                "[DAOS_MD_DEBUG] InstallMDForTimestep Step=%zu pgstart=%zu Position=%zu "
                "WriterCount=%zu perRank=%d\n",
                Step, pgstart, Position, (size_t)WriterCount, (int)m_PerRankMetadata);

    if (!m_PerRankMetadata)
    {
        // ---- aggregated mode (Phase 1): blob = [MetaEncode][step_start u64] ----
        // m_Metadata layout: [total_mdsize][mdsize×N][attr_size×N][meta][attr]
        size_t MDPosition = Position + 2 * sizeof(uint64_t) * WriterCount;
        for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
        {
            size_t ThisMDSizeFull = helper::ReadValue<uint64_t>(m_Metadata.m_Buffer, Position,
                                                                m_Minifooter.IsLittleEndian);
            size_t ThisMDSize = ThisMDSizeFull - sizeof(uint64_t);
            char *ThisMD = m_Metadata.m_Buffer.data() + MDPosition;
            uint64_t stepStart = 0;
            std::memcpy(&stepStart, ThisMD + ThisMDSize, sizeof(uint64_t));
            stepStarts[WriterRank] = stepStart;
            if (mdDebug && Step == 0 && (WriterRank < 4 || WriterRank + 1 == WriterCount))
                fprintf(stderr,
                        "[DAOS_MD_DEBUG]   agg rank=%zu MDPosition=%zu ThisMDSizeFull=%zu "
                        "ThisMDSize=%zu first=%02x%02x%02x%02x\n",
                        WriterRank, MDPosition, ThisMDSizeFull, ThisMDSize,
                        (unsigned char)ThisMD[0], (unsigned char)ThisMD[1],
                        (unsigned char)ThisMD[2], (unsigned char)ThisMD[3]);
            try
            {
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
            }
            catch (const std::exception &)
            {
                // Pinpoint exactly which rank's blob the deserializer rejects
                // and dump its leading bytes -- distinguishes a garbage blob
                // (bad first bytes) from a deserializer-state bug (valid id,
                // still thrown).
                if (mdDebug)
                    fprintf(stderr,
                            "[DAOS_MD_DEBUG]   *** InstallMetaData THREW at rank=%zu "
                            "MDPosition=%zu ThisMDSizeFull=%zu ThisMDSize=%zu "
                            "first8=%02x%02x%02x%02x%02x%02x%02x%02x\n",
                            WriterRank, MDPosition, ThisMDSizeFull, ThisMDSize,
                            (unsigned char)ThisMD[0], (unsigned char)ThisMD[1],
                            (unsigned char)ThisMD[2], (unsigned char)ThisMD[3],
                            (unsigned char)ThisMD[4], (unsigned char)ThisMD[5],
                            (unsigned char)ThisMD[6], (unsigned char)ThisMD[7]);
                throw;
            }
            MDPosition += ThisMDSizeFull;
        }

        for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
        {
            // attribute metadata for timestep
            size_t ThisADSize = helper::ReadValue<uint64_t>(m_Metadata.m_Buffer, Position,
                                                            m_Minifooter.IsLittleEndian);
            char *ThisAD = m_Metadata.m_Buffer.data() + MDPosition;
            if (ThisADSize > 0)
                m_BP5Deserializer->InstallAttributeData(ThisAD, ThisADSize);
            MDPosition += ThisADSize;
        }
        return;
    }

    // ---- per-rank mode (Phase 2): blob = [MetaEncode][MetaMeta][Attr][24B footer] ----
    // m_Metadata layout (no attr-size layer): [total_mdsize][mdsize×N][blobs]
    size_t MDPosition = Position + sizeof(uint64_t) * WriterCount;
    constexpr size_t footerSize = 3 * sizeof(uint64_t);
    for (size_t WriterRank = 0; WriterRank < WriterCount; WriterRank++)
    {
        const size_t ThisBlobSize =
            helper::ReadValue<uint64_t>(m_Metadata.m_Buffer, Position, m_Minifooter.IsLittleEndian);
        char *blob = m_Metadata.m_Buffer.data() + MDPosition;
        // Read footer from the tail of the blob.
        uint64_t mmbSize = 0, attrSize = 0, stepStart = 0;
        std::memcpy(&mmbSize, blob + ThisBlobSize - footerSize, sizeof(uint64_t));
        std::memcpy(&attrSize, blob + ThisBlobSize - 2 * sizeof(uint64_t), sizeof(uint64_t));
        std::memcpy(&stepStart, blob + ThisBlobSize - sizeof(uint64_t), sizeof(uint64_t));
        stepStarts[WriterRank] = stepStart;
        const size_t metaSize = ThisBlobSize - mmbSize - attrSize - footerSize;

        if (mdDebug && Step == 0 && (WriterRank < 4 || WriterRank + 1 == WriterCount))
            fprintf(stderr,
                    "[DAOS_MD_DEBUG]   prr rank=%zu MDPosition=%zu ThisBlobSize=%zu metaSize=%zu "
                    "mmbSize=%llu attrSize=%llu first=%02x%02x%02x%02x\n",
                    WriterRank, MDPosition, ThisBlobSize, metaSize, (unsigned long long)mmbSize,
                    (unsigned long long)attrSize, (unsigned char)blob[0], (unsigned char)blob[1],
                    (unsigned char)blob[2], (unsigned char)blob[3]);

        // Install MetaMetaBlocks for this rank (count + (idLen,infoLen,id,info)+).
        if (mmbSize > 0)
        {
            char *mmbStart = blob + metaSize;
            size_t mp = 0;
            uint64_t mmbCount = 0;
            std::memcpy(&mmbCount, mmbStart + mp, sizeof(uint64_t));
            mp += sizeof(uint64_t);
            for (uint64_t k = 0; k < mmbCount; ++k)
            {
                format::BP5Base::MetaMetaInfoBlock MMI;
                std::memcpy(&MMI.MetaMetaIDLen, mmbStart + mp, sizeof(uint64_t));
                mp += sizeof(uint64_t);
                std::memcpy(&MMI.MetaMetaInfoLen, mmbStart + mp, sizeof(uint64_t));
                mp += sizeof(uint64_t);
                MMI.MetaMetaID = mmbStart + mp;
                mp += MMI.MetaMetaIDLen;
                MMI.MetaMetaInfo = mmbStart + mp;
                mp += MMI.MetaMetaInfoLen;
                // Each rank carries its own copy of the MMBs; with N ranks
                // defining the same variables that is N duplicates of each
                // format ID.  Install each unique ID exactly once.
                std::string idKey(MMI.MetaMetaID, MMI.MetaMetaIDLen);
                if (m_InstalledMetaMetaIDs.insert(std::move(idKey)).second)
                    m_BP5Deserializer->InstallMetaMetaData(MMI);
            }
        }

        // Install per-rank attribute block (if any).
        if (attrSize > 0)
        {
            char *attrStart = blob + metaSize + mmbSize;
            m_BP5Deserializer->InstallAttributeData(attrStart, attrSize);
        }

        // Install variable metadata.
        if (m_OpenMode == Mode::ReadRandomAccess)
        {
            m_BP5Deserializer->InstallMetaData(blob, metaSize, WriterRank, Step);
        }
        else
        {
            CALI_MARK_BEGIN("DaosReader::InstallMetaData");
            m_BP5Deserializer->InstallMetaData(blob, metaSize, WriterRank);
            CALI_MARK_END("DaosReader::InstallMetaData");
        }

        MDPosition += ThisBlobSize;
    }
}

StepStatus DaosReader::BeginStep(StepMode mode, const float timeoutSeconds)
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::BeginStep");

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

    // used to inquire for variables in streaming mode
    m_IO.m_ReadStreaming = true;

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

namespace
{
// One daos_array_read in flight.
struct ReadOp
{
    daos_event_t ev;
    daos_range_t rg;               // contiguous op
    std::vector<daos_range_t> rgs; // strided batch
    daos_array_iod_t iod;
    d_iov_t iov;
    std::vector<d_iov_t> iovs; // zero-copy strided batch: one per run
    d_sg_list_t sgl;
    size_t req = 0;
    size_t tokens = 0;
    // Strided: runs [firstRun, endRun) arrive packed in stage, in range order,
    // or straight into application memory when zeroCopy.
    size_t firstRun = 0, endRun = 0;
    bool zeroCopy = false;
    std::unique_ptr<char[]> stage;
    size_t stageCap = 0;
};

// Array chunks (dkeys) that [off, off+len) touches; DAOS registers one bulk
// buffer per chunk touched, so this is an op's cost against the budget.
size_t ChunksSpanned(size_t off, size_t len, size_t chunk)
{
    return len ? (off + len - 1) / chunk - off / chunk + 1 : 0;
}

double Elapsed(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b)
{
    return std::chrono::duration<double>(b - a).count();
}
} // namespace

/*
 * All DAOS reads of a step are issued from this one thread on one event queue.
 * Reader threads each calling daos_array_read synchronously leak registrations
 * until the node's pool is exhausted and the client double-frees (large reads at
 * 4 threads abort every run of 16+ steps; the same reads from one thread never
 * do), so concurrency comes from reads in flight instead of threads.
 *
 * In flight is bounded by m_ReadTokens, counted in array chunks touched -- the
 * unit the registration pool is consumed in.  Contiguous requests are split into
 * DAOS_READ_CHUNK_BYTES reads; strided ones into batches of runs bounded by
 * bytes and by array span (a 16 MiB column-stripe batch can otherwise span a
 * writer's whole block), each staged through its own contiguous buffer.
 *
 * The CPU work -- scattering staged runs and FinalizeGet's extraction -- runs on
 * copy workers that never call DAOS, so it overlaps the reads instead of
 * stalling the issuing thread.  A request is finalized once it is fully issued
 * and every read of it has landed.
 */
void DaosReader::PerformGets()
{
    PERFSTUBS_SCOPED_TIMER("DaosReader::PerformGets");
    using Clock = std::chrono::steady_clock;
    const auto tStart = Clock::now();
    size_t maxReadSize;
    auto ReadRequests = m_BP5Deserializer->GenerateReadRequests(false, &maxReadSize);
    const size_t nRequest = ReadRequests.size();

    static const size_t readChunk = []() -> size_t {
        const char *e = std::getenv("DAOS_READ_CHUNK_BYTES");
        const size_t v = e ? static_cast<size_t>(std::strtoull(e, nullptr, 10)) : 0;
        return v ? v : size_t{64} * 1024 * 1024;
    }();
    static const size_t batchBytesMax = []() -> size_t {
        const char *e = std::getenv("DAOS_SUBRANGE_BATCH_BYTES");
        const size_t v = e ? static_cast<size_t>(std::strtoull(e, nullptr, 10)) : 0;
        return v ? v : size_t{16} * 1024 * 1024;
    }();
    static const size_t batchSpanChunks = []() -> size_t {
        const char *e = std::getenv("DAOS_SUBRANGE_SPAN_CHUNKS");
        const size_t v = e ? static_cast<size_t>(std::strtoull(e, nullptr, 10)) : 0;
        return v ? v : 8;
    }();
    // Backstop on the descriptor array; must stay under DAOS_ARRAY_LIST_IO_LIMIT
    // (16384), which binds for ranges below DAOS_ARRAY_RG_LEN_THD (16 bytes).
    constexpr size_t maxBatchRanges = 1024;
    // Requests the deserializer cannot place in application memory read into a
    // span-sized scratch buffer; this many may be open at once.  Four keep the
    // copy workers busy on strided reads; each holds a span across steps.
    static const size_t scratchSlots = []() -> size_t {
        const char *e = std::getenv("DAOS_READ_SCRATCH_SLOTS");
        const size_t v = e ? static_cast<size_t>(std::strtoull(e, nullptr, 10)) : 0;
        return v ? v : 4;
    }();
    // Strided runs with a known application address go there as their own iovs
    // (no copy, one bulk segment per run) by default; DAOS_SUBRANGE_ZEROCOPY=0
    // routes them through the stage instead (one memcpy each, one segment).
    static const bool subrangeZeroCopy = []() {
        const char *e = std::getenv("DAOS_SUBRANGE_ZEROCOPY");
        return !e || std::atoi(e) != 0;
    }();

    if (m_ReadEQ.cookie == 0)
    {
        const int rc = daos_eq_create(&m_ReadEQ);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "PerformGets",
                                              "daos_eq_create failed rc=" + std::to_string(rc));
        }
    }

    // Scratch persists across steps: a fresh span-sized buffer per step costs a
    // page fault per page every step.  Uninitialized on purpose -- NdCopy only
    // visits bytes the reads wrote.
    if (m_ReadScratchCap < maxReadSize)
    {
        m_ReadScratch.clear();
        m_ReadScratchCap = maxReadSize;
    }

    struct ReqState
    {
        bool started = false;
        bool issued = false; // every read of the request has been submitted
        size_t next = 0;     // contiguous: next byte; strided: next run
        size_t arrayOffset = 0;
        size_t arrayChunk = 1;
        daos_handle_t aoh{};
        int scratch = -1; // scratch slot, or -1 when reading into app memory
        // Reads not yet landed, plus one held until the request is fully
        // issued; whoever drops it to zero queues the finalize.
        std::atomic<size_t> outstanding{0};
    };
    std::vector<ReqState> st(nRequest);
    std::vector<int> freeScratch; // guarded by mtx
    std::vector<std::unique_ptr<ReadOp>> idleOps;
    std::vector<std::unique_ptr<ReadOp>> returnedOps; // guarded by mtx
    std::unordered_map<daos_event_t *, std::unique_ptr<ReadOp>> busy;
    // Peak sgl segments in flight, a gauge of load on the bulk path.
    size_t segsInFlight = 0;
    // Signed: a read costlier than the whole budget is admitted alone and
    // drives this negative until it completes.
    std::ptrdiff_t tokensFree = static_cast<std::ptrdiff_t>(m_ReadTokens);
    size_t cur = 0;
    std::string err;       // issuing thread only
    std::string workerErr; // guarded by mtx
    size_t finalized = 0;  // guarded by mtx

    // Copy-worker queue.  A task is a landed strided op to scatter (op set), or
    // a request to finalize (op null).
    struct Task
    {
        ReadOp *op;
        size_t req;
    };
    std::mutex mtx;
    std::condition_variable workCv, doneCv;
    std::deque<Task> tasks;
    bool stopWorkers = false;
    double workerBusy = 0.0; // guarded by mtx

    auto lf_QueueFinalize = [&](size_t r) {
        std::lock_guard<std::mutex> g(mtx);
        tasks.push_back(Task{nullptr, r});
        workCv.notify_one();
    };
    auto lf_Release = [&](size_t r) {
        if (st[r].outstanding.fetch_sub(1) == 1)
        {
            lf_QueueFinalize(r);
        }
    };

    auto lf_Worker = [&]() {
        std::unique_lock<std::mutex> lk(mtx);
        while (true)
        {
            workCv.wait(lk, [&] { return stopWorkers || !tasks.empty(); });
            if (tasks.empty())
            {
                return;
            }
            Task t = tasks.front();
            tasks.pop_front();
            const bool skip = !workerErr.empty();
            lk.unlock();
            const auto t0 = Clock::now();
            std::string e;
            auto &Req = ReadRequests[t.req];
            if (t.op)
            {
                if (!skip)
                {
                    // Scatter the packed runs to where a whole-span read would
                    // have put them, which is what the extraction addresses.
                    const bool toApp = !Req.SubRangeDest.empty();
                    size_t packed = 0;
                    for (size_t j = t.op->firstRun; j < t.op->endRun; ++j)
                    {
                        char *dst = toApp ? Req.SubRangeDest[j]
                                          : Req.DestinationAddr + Req.SubRanges[j].first;
                        std::memcpy(dst, t.op->stage.get() + packed, Req.SubRanges[j].second);
                        packed += Req.SubRanges[j].second;
                    }
                }
            }
            else if (!skip)
            {
                try
                {
                    m_BP5Deserializer->FinalizeGet(Req, false);
                }
                catch (std::exception &ex)
                {
                    e = ex.what();
                }
            }
            const double dt = Elapsed(t0, Clock::now());
            lk.lock();
            workerBusy += dt;
            if (!e.empty() && workerErr.empty())
            {
                workerErr = e;
            }
            if (t.op)
            {
                returnedOps.emplace_back(t.op);
                lk.unlock();
                lf_Release(t.req);
                lk.lock();
            }
            else
            {
                if (st[t.req].scratch >= 0)
                {
                    freeScratch.push_back(st[t.req].scratch);
                    st[t.req].scratch = -1;
                }
                ++finalized;
            }
            doneCv.notify_all();
        }
    };
    std::vector<std::thread> workers;
    for (size_t i = 0; i < m_ReadCopyThreads; ++i)
    {
        workers.emplace_back(lf_Worker);
    }
    auto lf_StopWorkers = [&]() {
        {
            std::lock_guard<std::mutex> g(mtx);
            stopWorkers = true;
        }
        workCv.notify_all();
        for (auto &w : workers)
        {
            w.join();
        }
        workers.clear();
    };

    // Resolves where request r reads from and to.  False when it must wait for
    // a scratch slot; sets err on a malformed request.
    auto lf_Start = [&](size_t r) -> bool {
        auto &Req = ReadRequests[r];
        auto &s = st[r];
        if (Req.WriterRank >= m_DataArrayHandles.size())
        {
            err = "WriterRank " + std::to_string(Req.WriterRank) +
                  " has no entry in OID index (size=" + std::to_string(m_DataArrayHandles.size()) +
                  ")";
            return false;
        }
        auto it = m_RankStartDataPos.find(Req.Timestep);
        if (it == m_RankStartDataPos.end() || Req.WriterRank >= it->second.size())
        {
            err = "missing step-start for Timestep=" + std::to_string(Req.Timestep) +
                  " WriterRank=" + std::to_string(Req.WriterRank);
            return false;
        }
        if (!Req.SubRangeDest.empty())
        {
            // Every run lands in application memory; nothing left to extract.
            Req.DirectToAppMemory = true;
        }
        else if (!Req.DestinationAddr)
        {
            std::lock_guard<std::mutex> g(mtx);
            if (freeScratch.empty())
            {
                if (m_ReadScratch.size() >= scratchSlots)
                {
                    return false;
                }
                // new char[] is non-null even at size 0.
                m_ReadScratch.emplace_back(new char[m_ReadScratchCap]);
                freeScratch.push_back(static_cast<int>(m_ReadScratch.size() - 1));
            }
            s.scratch = freeScratch.back();
            freeScratch.pop_back();
            Req.DestinationAddr = m_ReadScratch[s.scratch].get();
        }
        s.aoh = m_DataArrayHandles[Req.WriterRank];
        s.arrayOffset = it->second[Req.WriterRank] + Req.StartOffset;
        s.arrayChunk = m_DataArrayChunk[Req.WriterRank] ? m_DataArrayChunk[Req.WriterRank] : 1;
        s.started = true;
        s.outstanding = 1;
        s.issued = Req.SubRanges.empty() ? Req.ReadLength == 0 : false;
        return true;
    };
    // Scratch slots held by requests that predate this step's reuse.
    {
        std::lock_guard<std::mutex> g(mtx);
        for (size_t i = 0; i < m_ReadScratch.size(); ++i)
        {
            freeScratch.push_back(static_cast<int>(i));
        }
    }

    // Describes request r's next read into op; returns its token cost.
    auto lf_Plan = [&](size_t r, ReadOp &op) -> size_t {
        auto &Req = ReadRequests[r];
        auto &s = st[r];
        op.req = r;
        if (Req.SubRanges.empty())
        {
            // Keep one read within the budget even when the budget is small.
            const size_t maxLen = std::max<size_t>(1, m_ReadTokens - 1) * s.arrayChunk;
            const size_t len = std::min({readChunk, maxLen, Req.ReadLength - s.next});
            op.rg.rg_idx = s.arrayOffset + s.next;
            op.rg.rg_len = len;
            op.iod.arr_nr = 1;
            op.iod.arr_rgs = &op.rg;
            op.iov.iov_buf = Req.DestinationAddr + s.next;
            op.iov.iov_buf_len = len;
            op.iov.iov_len = len;
            op.zeroCopy = false;
            op.sgl.sg_nr = 1;
            op.sgl.sg_iovs = &op.iov;
            op.tokens = ChunksSpanned(op.rg.rg_idx, len, s.arrayChunk);
            return op.tokens;
        }
        const auto &runs = Req.SubRanges;
        const size_t first = s.next;
        const size_t firstChunk = (s.arrayOffset + runs[first].first) / s.arrayChunk;
        size_t i = first, bytes = 0, lastChunk = firstChunk;
        op.rgs.clear();
        while (i < runs.size() && op.rgs.size() < maxBatchRanges)
        {
            const size_t end = s.arrayOffset + runs[i].first + runs[i].second;
            const size_t endChunk = runs[i].second ? (end - 1) / s.arrayChunk : firstChunk;
            if (bytes && (bytes + runs[i].second > batchBytesMax ||
                          endChunk - firstChunk >= batchSpanChunks))
            {
                break;
            }
            daos_range_t rg;
            rg.rg_idx = s.arrayOffset + runs[i].first;
            rg.rg_len = runs[i].second;
            op.rgs.push_back(rg);
            bytes += runs[i].second;
            lastChunk = std::max(lastChunk, endChunk);
            ++i;
        }
        op.firstRun = first;
        op.endRun = i;
        if (bytes == 0)
        {
            // A zero-byte batch would hand DAOS an sgl at an unallocated stage.
            err = "zero-byte strided batch at run " + std::to_string(first) + " of " +
                  std::to_string(runs.size());
            return 0;
        }
        op.iod.arr_nr = static_cast<daos_size_t>(op.rgs.size());
        op.iod.arr_rgs = op.rgs.data();
        op.tokens = lastChunk - firstChunk + 1;
        op.zeroCopy = subrangeZeroCopy && !Req.SubRangeDest.empty();
        if (op.zeroCopy)
        {
            op.iovs.resize(i - first);
            for (size_t j = first; j < i; ++j)
            {
                d_iov_set(&op.iovs[j - first], Req.SubRangeDest[j], runs[j].second);
            }
            op.sgl.sg_nr = static_cast<uint32_t>(op.iovs.size());
            op.sgl.sg_iovs = op.iovs.data();
            return op.tokens;
        }
        if (op.stageCap < bytes)
        {
            op.stage.reset();
            op.stageCap = 0;
            op.stage.reset(new char[bytes]);
            op.stageCap = bytes;
        }
        op.iov.iov_buf = op.stage.get();
        op.iov.iov_buf_len = bytes;
        op.iov.iov_len = bytes;
        op.sgl.sg_nr = 1;
        op.sgl.sg_iovs = &op.iov;
        return op.tokens;
    };

    double pollWait = 0.0;
    bool eqDead = false;
    while (true)
    {
        {
            std::lock_guard<std::mutex> g(mtx);
            for (auto &op : returnedOps)
            {
                idleOps.push_back(std::move(op));
            }
            returnedOps.clear();
            if (err.empty() && !workerErr.empty())
            {
                err = workerErr;
            }
        }

        // Issue in request order while the budget allows; always admit one read
        // when nothing is in flight, so a read costlier than the budget runs alone.
        while (err.empty() && cur < nRequest)
        {
            auto &s = st[cur];
            if (!s.started && !lf_Start(cur))
            {
                break; // waiting for scratch, or err set
            }
            if (s.issued)
            {
                lf_Release(cur); // drop the issuing hold
                ++cur;
                continue;
            }
            std::unique_ptr<ReadOp> op;
            if (idleOps.empty())
            {
                op.reset(new ReadOp);
            }
            else
            {
                op = std::move(idleOps.back());
                idleOps.pop_back();
            }
            const size_t tokens = lf_Plan(cur, *op);
            if (!err.empty())
            {
                idleOps.push_back(std::move(op));
                break;
            }
            if (static_cast<std::ptrdiff_t>(tokens) > tokensFree && !busy.empty())
            {
                idleOps.push_back(std::move(op));
                break;
            }
            op->sgl.sg_nr_out = 0;
            int rc = daos_event_init(&op->ev, m_ReadEQ, nullptr);
            if (!rc)
            {
                rc = daos_array_read(s.aoh, DAOS_TX_NONE, &op->iod, &op->sgl, &op->ev);
                if (rc)
                {
                    daos_event_fini(&op->ev);
                }
            }
            if (rc)
            {
                err = "daos_array_read submit failed rc=" + std::to_string(rc) +
                      " WriterRank=" + std::to_string(ReadRequests[cur].WriterRank);
                idleOps.push_back(std::move(op));
                break;
            }
            tokensFree -= static_cast<std::ptrdiff_t>(tokens);
            ++s.outstanding;
            segsInFlight += op->sgl.sg_nr;
            m_ReadPeakSegs = std::max(m_ReadPeakSegs, segsInFlight);
            if (ReadRequests[cur].SubRanges.empty())
            {
                s.next += op->rg.rg_len;
                s.issued = s.next >= ReadRequests[cur].ReadLength;
            }
            else
            {
                s.next = op->endRun;
                s.issued = s.next >= ReadRequests[cur].SubRanges.size();
            }
            daos_event_t *key = &op->ev;
            busy.emplace(key, std::move(op));
            if (s.issued)
            {
                lf_Release(cur);
                ++cur;
            }
        }

        if (busy.empty())
        {
            // Nothing to reap: wait for the workers to free a scratch slot or
            // finish, or stop if an error leaves nothing more to issue.
            std::unique_lock<std::mutex> lk(mtx);
            if (finalized == nRequest || !err.empty() || !workerErr.empty())
            {
                break;
            }
            const size_t f = finalized, nf = freeScratch.size();
            doneCv.wait(lk, [&] {
                return finalized != f || freeScratch.size() != nf || !workerErr.empty();
            });
            continue;
        }

        daos_event_t *doneEv[64];
        const auto tp = Clock::now();
        const int nd = daos_eq_poll(m_ReadEQ, 1, DAOS_EQ_WAIT, 64, doneEv);
        pollWait += Elapsed(tp, Clock::now());
        if (nd < 0)
        {
            eqDead = true;
            err = "daos_eq_poll failed rc=" + std::to_string(nd);
            break;
        }
        for (int k = 0; k < nd; ++k)
        {
            auto bit = busy.find(doneEv[k]);
            if (bit == busy.end())
            {
                continue; // not ours; cannot happen on a private EQ
            }
            std::unique_ptr<ReadOp> op = std::move(bit->second);
            busy.erase(bit);
            const size_t r = op->req;
            if (op->ev.ev_error && err.empty())
            {
                err = "daos_array_read failed rc=" + std::to_string(op->ev.ev_error) +
                      " WriterRank=" + std::to_string(ReadRequests[r].WriterRank);
            }
            daos_event_fini(&op->ev);
            tokensFree += static_cast<std::ptrdiff_t>(op->tokens);
            segsInFlight -= op->sgl.sg_nr;
            if (!ReadRequests[r].SubRanges.empty() && !op->zeroCopy)
            {
                std::lock_guard<std::mutex> g(mtx);
                tasks.push_back(Task{op.release(), r});
                workCv.notify_one();
            }
            else
            {
                idleOps.push_back(std::move(op));
                lf_Release(r);
            }
        }
    }

    if (eqDead)
    {
        // The EQ is unusable, so in-flight reads can never be waited for; leak
        // their descriptors and every buffer they may target rather than free
        // memory DAOS may still touch.
        for (auto &b : busy)
        {
            b.second.release();
        }
        for (auto &sb : m_ReadScratch)
        {
            sb.release();
        }
        m_ReadScratch.clear();
        m_ReadScratchCap = 0;
    }
    lf_StopWorkers();
    for (auto &op : returnedOps)
    {
        idleOps.push_back(std::move(op));
    }
    if (err.empty())
    {
        err = workerErr;
    }

    m_ReadPollWait += pollWait;
    m_ReadCopyBusy += workerBusy;
    m_ReadGetsTime += Elapsed(tStart, Clock::now());
    if (std::getenv("DAOS_THREAD_DEBUG") && m_Comm.Rank() == 0 && m_ReadGetsCalls == 0)
    {
        fprintf(stderr,
                "[DAOS_READ_DEBUG] PerformGets: nRequest=%zu tokens=%zu chunk=%zu "
                "maxReadSize=%zu copyThreads=%zu\n",
                nRequest, m_ReadTokens, readChunk, maxReadSize, m_ReadCopyThreads);
    }

    ++m_ReadGetsCalls;

    if (!err.empty())
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "PerformGets", err);
    }
    if (finalized != nRequest)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "PerformGets",
                                          "finalized " + std::to_string(finalized) + " of " +
                                              std::to_string(nRequest) + " read requests");
    }

    // clear pending requests inside deserializer
    {
        std::vector<adios2::format::BP5Deserializer::ReadRequest> empty;
        m_BP5Deserializer->FinalizeGets(empty);
    }
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

    if (m_IO.m_TransportsParameters.size() > 1)
    {
        helper::Throw<std::invalid_argument>(
            "Engine", "DaosReader", "Init",
            "DAOS engine accepts only one transport per IO; got " +
                std::to_string(m_IO.m_TransportsParameters.size()) +
                ". Multiple parallel transports per IO are no longer supported.");
    }

    // if IO was involved in reading before this flag may be true now
    m_IO.m_ReadStreaming = false;
    m_ReaderIsRowMajor = (m_IO.m_ArrayOrder == ArrayOrdering::RowMajor);
    InitParameters();
    InitTransports();
    CALI_MARK_BEGIN("DaosReader::InitDAOS");
    InitDAOS();
    CALI_MARK_END("DaosReader::InitDAOS");

    // Per-rank DAOS data array: data_oids.txt is required (written by
    // DaosWriter::PersistDataOidsIndex).  Older datasets that pre-date
    // the per-rank-array writer are no longer supported.
    {
        std::string indexPath = m_Name + "/data_oids.txt";
        struct stat st;
        if (::stat(indexPath.c_str(), &st) != 0)
        {
            helper::Throw<std::runtime_error>(
                "Engine", "DaosReader", "InitDAOS",
                "missing required OID index " + indexPath +
                    " (dataset must be produced by current DaosWriter)");
        }
        LoadDataOidsIndex();
        OpenDataArrays();
    }

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

    // Registration budget for reads in flight, in array chunks.  The pool it
    // guards is node-wide (aborts at ~15.7k), so a node budget is split among
    // the ranks sharing the node.  DAOS_READ_TOKENS sets the per-rank value
    // directly.  The Threads parameter no longer applies: all reads are issued
    // from one thread (see PerformGets).
    helper::Comm nodeComm = m_Comm.GroupByShm("creating per-node comm at DAOS Open(read)");
    const size_t nodeSize = std::max<size_t>(1, static_cast<size_t>(nodeComm.Size()));
    const char *perRank = std::getenv("DAOS_READ_TOKENS");
    const char *perNode = std::getenv("DAOS_READ_NODE_TOKENS");
    const size_t nodeTokens = perNode ? std::strtoull(perNode, nullptr, 10) : 8192;
    m_ReadTokens = perRank ? std::strtoull(perRank, nullptr, 10) : nodeTokens / nodeSize;
    m_ReadTokens = std::max<size_t>(m_ReadTokens, 16);
    // Copy workers share the node's cores with the other ranks: ~32 per node.
    const char *copyThr = std::getenv("DAOS_READ_COPY_THREADS");
    const size_t copyThreads = copyThr ? std::strtoull(copyThr, nullptr, 10) : 32 / nodeSize;
    m_ReadCopyThreads = std::min<size_t>(std::max<size_t>(copyThreads, 1), 16);
    if (std::getenv("DAOS_THREAD_DEBUG") && m_Comm.Rank() == 0)
    {
        fprintf(stderr, "[DAOS_READ_DEBUG] NodeSize=%zu -> tokens/rank=%zu copyThreads=%zu\n",
                nodeSize, m_ReadTokens, m_ReadCopyThreads);
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

size_t DaosReader::OpenWithTimeout(std::shared_ptr<Transport> &file, const std::string &fileName,
                                   const TimePoint &timeoutInstant, const Seconds &pollSeconds,
                                   std::string &lasterrmsg /*INOUT*/)
{
    size_t flag = 1; // 0 = OK, opened file, 1 = timeout, 2 = error
    do
    {
        try
        {
            errno = 0;
            const bool profile = false;
            file = transport::OpenFile(m_Comm, fileName, adios2::Mode::Read,
                                       m_IO.m_TransportsParameters[0], profile);
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

        flag = OpenWithTimeout(m_MetadataIndexFile, metadataIndexFile, timeoutInstant, pollSeconds,
                               lasterrmsg);
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

            flag = OpenWithTimeout(m_MetadataFile, metadataFile, timeoutInstant, pollSeconds,
                                   lasterrmsg);
            if (flag != 0)
            {
                /* Close the metadata index table */
                m_MetadataIndexFile->Close();
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

                flag = OpenWithTimeout(m_MetaMetadataFile, metametadataFile, timeoutInstant,
                                       pollSeconds, lasterrmsg);
                if (flag != 0)
                {
                    /* Close the metametadata index table */
                    m_MetadataIndexFile->Close();
                    m_MetadataFile->Close();
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
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "array_oh_share",
                                              "daos_array_local2global failed rc=" +
                                                  std::to_string(rc));
        }
    }

    /** broadcast size of global handle to all peers */
    m_Comm.Bcast((uint64_t *)&ghdl.iov_buf_len, 1, 0);

    /** allocate buffer for global pool handle */
    // Unchecked, this hands DAOS an iov claiming iov_len bytes at a NULL buffer.
    ghdl.iov_buf = malloc(ghdl.iov_buf_len);
    if (ghdl.iov_buf == NULL)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "array_oh_share",
                                          "out of memory for a " +
                                              std::to_string(ghdl.iov_buf_len) +
                                              " byte global array handle");
    }
    ghdl.iov_len = ghdl.iov_buf_len;

    if (m_Comm.Rank() == 0)
    {
        /** generate actual global handle to share with peer tasks */
        rc = daos_array_local2global(*oh, &ghdl);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "array_oh_share",
                                              "daos_array_local2global failed rc=" +
                                                  std::to_string(rc));
        }
    }

    /** broadcast global handle to all peers */
    m_Comm.Bcast((char *)ghdl.iov_buf, ghdl.iov_len, 0);

    if (m_Comm.Rank() != 0)
    {
        /** unpack global handle */
        rc = daos_array_global2local(coh, ghdl, 0, oh);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "array_oh_share",
                                              "daos_array_global2local failed rc=" +
                                                  std::to_string(rc));
        }
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

/// Pick the metadata-layout from the DAOS_METADATA_LAYOUT environment
/// variable.  Defaults to ARRAY_1MB_ALIGNED when unset.  An unrecognized
/// value is rejected so a typo doesn't silently fall back.
void DaosReader::SetMetadataLayout()
{
    const char *env = std::getenv("DAOS_METADATA_LAYOUT");
    if (!env)
    {
        metadataLayout = MetadataLayout::ARRAY_1MB_ALIGNED;
        return;
    }

    std::string layoutStr(env);
    std::transform(layoutStr.begin(), layoutStr.end(), layoutStr.begin(), ::tolower);
    if (layoutStr == "array")
    {
        metadataLayout = MetadataLayout::ARRAY;
    }
    else if (layoutStr == "array-1mb-aligned")
    {
        metadataLayout = MetadataLayout::ARRAY_1MB_ALIGNED;
    }
    else if (layoutStr == "kv")
    {
        metadataLayout = MetadataLayout::KV;
    }
    else
    {
        helper::Throw<std::invalid_argument>("Engine", "DaosReader", "SetMetadataLayout",
                                             "DAOS_METADATA_LAYOUT='" + layoutStr +
                                                 "' is not recognized; expected one of "
                                                 "'array', 'array-1mb-aligned', 'kv'");
    }
}

// Set m_PoolName and m_ContName from the environment variables DAOS_POOL and DAOS_CONT
void DaosReader::SetPoolAndContName()
{
    const char *pool = std::getenv("DAOS_POOL");
    const char *cont = std::getenv("DAOS_CONT");
    if (!pool || !cont)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosReader", "SetPoolAndContName",
            "DAOS_POOL and DAOS_CONT environment variables must both be set");
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
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "ReadObjectIDsFromFile",
                                          "fopen failed for " + OIDFileName + ": " +
                                              std::string(strerror(errno)));
    }
    auto badRead = [&]() {
        fclose(fp);
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "ReadObjectIDsFromFile",
                                          "malformed OID file " + OIDFileName);
    };
    if (metadataLayout == MetadataLayout::ARRAY ||
        metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {
        if (fscanf(fp, "%" SCNu64 "\n%" SCNu64 "\n", &oid.hi, &oid.lo) != 2)
            badRead();
        if (fscanf(fp, "%" SCNu64 "\n%" SCNu64 "\n", &mdsize_oid.hi, &mdsize_oid.lo) != 2)
            badRead();
    }
    else if (metadataLayout == MetadataLayout::KV)
    {
        if (fscanf(fp, "%" SCNu64 "\n%" SCNu64 "\n", &oid.hi, &oid.lo) != 2)
            badRead();
    }
    fclose(fp);
}

void DaosReader::OpenDAOSObjects()
{
    int rc;
    if (metadataLayout == MetadataLayout::ARRAY ||
        metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {
        daos_size_t cell_size = 1;
        daos_size_t chunk_size = 1048576;
        rc =
            daos_array_open(coh, oid, DAOS_TX_NONE, DAOS_OO_RO, &cell_size, &chunk_size, &oh, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "OpenDAOSObjects",
                                              "daos_array_open failed rc=" + std::to_string(rc));
        }

        rc = daos_kv_open(coh, mdsize_oid, DAOS_OO_RO, &mdsize_oh, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "OpenDAOSObjects",
                                              "daos_kv_open (mdsize) failed rc=" +
                                                  std::to_string(rc));
        }
    }
    else if (metadataLayout == MetadataLayout::KV)
    {
        // Open KV object
        CALI_MARK_BEGIN("DaosReader::daos_kv_open");
        rc = daos_kv_open(coh, oid, DAOS_OO_RO, &oh, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "OpenDAOSObjects",
                                              "daos_kv_open failed rc=" + std::to_string(rc));
        }
        CALI_MARK_END("DaosReader::daos_kv_open");

        // Create event queue;
        CALI_MARK_BEGIN("DaosReader::EventQueueCreation");
        rc = daos_eq_create(&eq);
        CALI_MARK_END("DaosReader::EventQueueCreation");
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "OpenDAOSObjects",
                                              "daos_eq_create failed rc=" + std::to_string(rc));
        }

        // Init events
        for (int i = 0; i < MAX_KV_GET_REQS; i++)
        {
            CALI_MARK_BEGIN("DaosReader::EventInitialization");
            rc = daos_event_init(&ev[i], eq, NULL);
            CALI_MARK_END("DaosReader::EventInitialization");
            if (rc)
            {
                helper::Throw<std::runtime_error>("Engine", "DaosReader", "OpenDAOSObjects",
                                                  "daos_event_init failed rc=" +
                                                      std::to_string(rc));
            }
        }
    }
}

void DaosReader::InitDAOS()
{
    // Rank 0 - Connect to DAOS pool, and open container
    int rc;
    CALI_MARK_BEGIN("DaosReader::daos_init");
    rc = daos_init();
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "InitDAOS",
                                          "daos_init failed rc=" + std::to_string(rc));
    }
    CALI_MARK_END("DaosReader::daos_init");

    rc = gethostname(node, sizeof(node));
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "InitDAOS",
                                          "gethostname failed (buffer too small)");
    }

    SetMetadataLayout();
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
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "InitDAOS",
                                              "daos_pool_connect failed rc=" + std::to_string(rc));
        }
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
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "InitDAOS",
                                              "daos_cont_open failed rc=" + std::to_string(rc));
        }
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
        const size_t metadataIndexFileSize = m_MetadataIndexFile->GetSize();
        newIdxSize = metadataIndexFileSize - m_MDIndexFileAlreadyReadSize;
        if (metadataIndexFileSize > m_MDIndexFileAlreadyReadSize)
        {
            m_MetadataIndex.m_Buffer.resize(newIdxSize);
            m_MetadataIndexFile->Read(m_MetadataIndex.m_Buffer.data(), newIdxSize,
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
            //          m_MetadataFile->Read(m_Metadata.m_Buffer.data() + mempos,
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
            const size_t metametadataFileSize = m_MetaMetadataFile->GetSize();
            if (metametadataFileSize > m_MetaMetaDataFileAlreadyReadSize)
            {
                const size_t newMMDSize = metametadataFileSize - m_MetaMetaDataFileAlreadyReadSize;
                m_MetaMetadata.Resize(metametadataFileSize, "(re)allocating meta-meta-data buffer, "
                                                            "in call to DaosReader Open");
                m_MetaMetadataFile->Read(m_MetaMetadata.m_Buffer.data() +
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

        // Phase-2 mode flag (byte 41).  0 = aggregated (Phase 1), 1 =
        // per-rank metadata layout.  Used by ReadMetadata /
        // InstallMetadataForTimestep to dispatch.
        position = m_PerRankMetadataFlagPosition;
        const uint8_t perRankFlag =
            helper::ReadValue<uint8_t>(buffer, position, m_Minifooter.IsLittleEndian);
        m_PerRankMetadata = (perRankFlag == 1);

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
            // Phase-1: no FlushCount, no per-writer block.  Each rank's
            // step-start data offset lives in its own metadata-blob
            // trailer in DAOS, not in md.idx.

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
                // absolute pos in file before read
                ptrs.push_back(MetadataPos);
                m_MetadataIndexTable[m_StepsCount] = ptrs;
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
        auto fsize = m_MetadataIndexFile->GetSize();
        if (fsize >= m_IndexHeaderSize)
        {
            std::vector<char> header(m_IndexHeaderSize, '\0');
            m_MetadataIndexFile->Read(header.data(), m_IndexHeaderSize, 0);
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

    // Collaborative teardown.  The pool/container handles are shared across
    // ranks via daos_{pool,cont}_global2local: rank 0 owns the real pool
    // connection and container open; the other ranks hold lightweight local
    // copies.  All data/metadata reads use those handles.  If any rank reaches
    // Close first and tears them down (daos_cont_close / rank-0
    // daos_pool_disconnect below), a straggler still reading the last step
    // loses the shared handle mid-read -> DER_NO_HDL(-1002).  Barrier here so
    // no handle is released until every rank has finished all its reads.
    m_Comm.Barrier();

    if (m_Comm.Rank() == 0)
    {
        m_MetadataFile->Close();
        m_MetadataIndexFile->Close();
        m_MetaMetadataFile->Close();
    }
    CloseDataArrays();
    if (std::getenv("DAOS_THREAD_DEBUG") && m_Comm.Rank() == 0 && m_ReadGetsCalls)
    {
        fprintf(stderr,
                "[DAOS_READ_DEBUG] rank0 totals: PerformGets=%.3f s pollWait=%.3f s "
                "copyBusy=%.3f s (over %zu workers) steps=%zu peakSegs=%zu\n",
                m_ReadGetsTime, m_ReadPollWait, m_ReadCopyBusy, m_ReadCopyThreads, m_ReadGetsCalls,
                m_ReadPeakSegs);
    }
    m_ReadScratch.clear();
    m_ReadScratchCap = 0;
    // PerformGets reaps every read before returning, so the queue is empty.
    if (m_ReadEQ.cookie != 0)
    {
        daos_eq_destroy(m_ReadEQ, 0);
        m_ReadEQ = {};
    }

    // Release DAOS handles in reverse of init order; mirrors DaosWriter::DoClose.
    if (oh.cookie != 0)
    {
        if (metadataLayout == MetadataLayout::ARRAY ||
            metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
            daos_array_close(oh, NULL);
        else if (metadataLayout == MetadataLayout::KV)
            daos_kv_close(oh, NULL);
        oh = {};
    }
    if (mdsize_oh.cookie != 0)
    {
        daos_kv_close(mdsize_oh, NULL);
        mdsize_oh = {};
    }
    if (coh.cookie != 0)
    {
        daos_cont_close(coh, NULL);
        coh = {};
    }
    // Ensure every rank has released its container handle before rank 0 drops
    // the shared pool connection (rank-0 disconnect invalidates the pool for
    // all global2local holders).
    m_Comm.Barrier();
    if (m_Comm.Rank() == 0 && poh.cookie != 0)
    {
        daos_pool_disconnect(poh, NULL);
        poh = {};
    }
    // Don't daos_fini() — see matching comment in DaosWriter::DoClose.
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
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "daos_handle_share",
                                              "local2global (size) failed rc=" +
                                                  std::to_string(rc));
        }
    }

    /** broadcast size of global handle to all peers */
    m_Comm.Bcast((uint64_t *)&ghdl.iov_buf_len, 1, 0);
    CALI_MARK_END("DaosReader::local2global+broadcast_sizeofhandle");

    /** allocate buffer for global pool handle */
    // Unchecked, this hands DAOS an iov claiming iov_len bytes at a NULL buffer.
    ghdl.iov_buf = malloc(ghdl.iov_buf_len);
    if (ghdl.iov_buf == NULL)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosReader", "daos_handle_share",
            "out of memory for a " + std::to_string(ghdl.iov_buf_len) + " byte global handle");
    }
    ghdl.iov_len = ghdl.iov_buf_len;

    CALI_MARK_BEGIN("DaosReader::local2global+broadcast_handle");
    if (m_Comm.Rank() == 0)
    {
        /** generate actual global handle to share with peer tasks */
        if (type == DaosReader::HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else
            rc = daos_cont_local2global(*hdl, &ghdl);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "daos_handle_share",
                                              "local2global failed rc=" + std::to_string(rc));
        }
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
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "daos_handle_share",
                                              "global2local failed rc=" + std::to_string(rc));
        }
    }

    free(ghdl.iov_buf);

    m_Comm.Barrier();
    CALI_MARK_END("DaosReader::global2local+barrier");
}

void DaosReader::LoadDataOidsIndex()
{
    // Parse <m_Name>/data_oids.txt written by DaosWriter::PersistDataOidsIndex.
    // Format: comment lines starting with '#', then one record per line:
    //   "<rank> <oid_lo> <oid_hi>"
    const std::string indexPath = m_Name + "/data_oids.txt";
    std::ifstream f(indexPath);
    if (!f)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "LoadDataOidsIndex",
                                          "failed to open " + indexPath);
    }

    // Two-pass: find max rank to size the vector, then populate.
    std::vector<std::tuple<int, uint64_t, uint64_t>> records;
    std::string line;
    int maxRank = -1;
    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream ls(line);
        int r;
        uint64_t lo, hi;
        if (!(ls >> r >> lo >> hi))
            continue;
        records.emplace_back(r, lo, hi);
        if (r > maxRank)
            maxRank = r;
    }
    if (maxRank < 0)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosReader", "LoadDataOidsIndex",
                                          "no records parsed from " + indexPath);
    }

    m_DataOids.assign(static_cast<size_t>(maxRank + 1), daos_obj_id_t{});
    for (auto &rec : records)
    {
        int r = std::get<0>(rec);
        m_DataOids[r].lo = std::get<1>(rec);
        m_DataOids[r].hi = std::get<2>(rec);
    }
}

void DaosReader::OpenDataArrays()
{
    m_DataArrayHandles.assign(m_DataOids.size(), daos_handle_t{});
    m_DataArrayChunk.assign(m_DataOids.size(), 0);
    daos_size_t cell_size_out = 0;
    for (size_t r = 0; r < m_DataOids.size(); ++r)
    {
        int rc = daos_array_open(coh, m_DataOids[r], DAOS_TX_NONE, DAOS_OO_RO, &cell_size_out,
                                 &m_DataArrayChunk[r], &m_DataArrayHandles[r], NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosReader", "OpenDataArrays",
                                              "daos_array_open failed for rank " +
                                                  std::to_string(r) + " rc=" + std::to_string(rc));
        }
    }
}

void DaosReader::CloseDataArrays()
{
    for (auto &h : m_DataArrayHandles)
    {
        if (h.cookie != 0)
        {
            daos_array_close(h, NULL);
            h = {};
        }
    }
    m_DataArrayHandles.clear();
    m_DataArrayChunk.clear();
    m_DataOids.clear();
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
