/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DaosWriter.h"
#include "DaosWriter.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/helper/adiosMath.h"      // SetWithinLimit
#include "adios2/helper/adiosMemory.h"    // NdCopy
#include "adios2/toolkit/format/bp5/BP5Helper.h"
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/chunk/DaosChunkV.h"
#include "adios2/toolkit/transport/OpenFile.h"
#include "adios2/toolkit/transport/file/FileFStream.h"

#include <adios2-perfstubs-interface.h>
#include <adios2sys/SystemTools.hxx>
#include <daos_array.h>

#include <chrono>
#include <cstdlib> // getenv
#include <cstring> // memcpy
#include <ctime>
#include <fstream>
#include <iomanip> // setw
#include <iostream>
#include <memory> // make_shared

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

DaosWriter::DaosWriter(IO &io, const std::string &name, const Mode mode, helper::Comm comm)
: Engine("DaosWriter", io, name, mode, std::move(comm)), m_BP5Serializer(), m_Profiler(m_Comm)
{
    m_EngineStart = Now();
    PERFSTUBS_SCOPED_TIMER("DaosWriter::Open");
    m_IO.m_ReadStreaming = false;

    Init();
    m_IsOpen = true;
}

StepStatus DaosWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    // Close the BetweenSteps timer (started at end of last EndStep).  Skip
    // on the very first BeginStep since there was no prior EndStep.  This
    // attributes iotest's data-generation gap to "BetweenSteps" so it can
    // be subtracted when comparing against the bench.
    if (m_WriterStep > 0)
    {
        m_Profiler.Stop("BetweenSteps");
    }

    profiling::ProfilerGuard bs(m_Profiler, "BS");

    if (m_BetweenStepPairs)
    {
        helper::Throw<std::logic_error>("Engine", "DaosWriter", "BeginStep",
                                        "BeginStep() is called a second time "
                                        "without an intervening EndStep()");
    }

    m_BetweenStepPairs = true;

    if ((m_WriterStep == 0) && m_Parameters.UseOneTimeAttributes)
    {
        const auto &attributes = m_IO.GetAttributes();

        for (const auto &attributePair : attributes)
        {
            m_BP5Serializer.OnetimeMarshalAttribute(*(attributePair.second));
        }
        // Mirrors BP5Writer: prevents EndStep::MarshalAttributes() from
        // running the regular MarshalAttribute path on top of these,
        // which double-registers attributes through two different
        // BP5Serializer code paths and produces an attribute block with
        // a different FFS format ID than BP5 does.
        m_MarshalAttributesNecessary = false;
    }

    // DaosChunkV submits async daos_array_writes during AddToVec at
    // chunk-finalize boundaries (and immediately for deferred-extern
    // entries).  EndStep / WriteData drains.  The event queue is
    // engine-lifetime (created once in OpenDataArray) —
    // daos_eq_create is ~300 ms so doing it per-step was a major
    // BeginStep bottleneck.
    //
    // m_DataBuf is engine-lifetime: allocated once on the first
    // step, Reset() between steps so chunk pointers stay stable across
    // steps.  Stable addresses keep the libfabric MR cache hot and
    // avoid per-step first-touch page-fault zeroing — empirically
    // ~30% per-iter savings vs fresh chunks at this workload.
    if (!m_DataBuf)
    {
        m_DataBuf.reset(new format::DaosChunkV(
            "DaosWriter", false, m_BP5Serializer.m_BufferAlign, m_BP5Serializer.m_BufferBlockSize,
            m_Parameters.BufferChunkSize, m_DataArrayOH, m_DataEQ, m_DataArrayCursor, m_Profiler));
    }
    else
    {
        m_DataBuf->Reset();
        m_DataBuf->SetBaseArrayOffset(m_DataArrayCursor);
    }
    m_BP5Serializer.InitStep(m_DataBuf.get());
    m_ThisTimestepDataSize = 0;
    // Snapshot the cursor at step begin.  Phase-1 design: each rank's
    // metadata blob carries its own step-start array offset as a trailer,
    // so the reader can compute arrayOffset = step_start + StartOffset.
    m_StepStartDataPos = m_DataArrayCursor;

    return StepStatus::OK;
}

size_t DaosWriter::CurrentStep() const { return m_WriterStep; }

void DaosWriter::PerformPuts()
{
    PERFSTUBS_SCOPED_TIMER("DaosWriter::PerformPuts");
    m_Profiler.Start("PP");
    m_BP5Serializer.PerformPuts(m_Parameters.AsyncWrite);
    m_Profiler.Stop("PP");
    return;
}

void DaosWriter::WriteMetaMetadata(
    const std::vector<format::BP5Base::MetaMetaInfoBlock> MetaMetaBlocks)
{
    profiling::ProfilerGuard g(m_Profiler, "MD_Posix");
    for (auto &b : MetaMetaBlocks)
    {
        m_MetaMetadataFile->Write((char *)&b.MetaMetaIDLen, sizeof(size_t));
        m_MetaMetadataFile->Write((char *)&b.MetaMetaInfoLen, sizeof(size_t));
        m_MetaMetadataFile->Write((char *)b.MetaMetaID, b.MetaMetaIDLen);
        m_MetaMetadataFile->Write((char *)b.MetaMetaInfo, b.MetaMetaInfoLen);
    }
}

uint64_t DaosWriter::WriteAttributes(const std::vector<core::iovec> &AttributeBlocks)
{
    profiling::ProfilerGuard g(m_Profiler, "MD_Posix");
    uint64_t MDataTotalSize = 0;
    uint64_t MetaDataSize = 0;
    std::vector<uint64_t> AttrSizeVector;
    for (auto &b : AttributeBlocks)
    {
        MDataTotalSize += sizeof(uint64_t) + b.iov_len;
        AttrSizeVector.push_back(b.iov_len);
    }
    // Pad AttrSizeVector to one entry per writer rank, matching BP5's
    // SelectiveAggregationMetadata layout that DaosReader's
    // InstallMetadataForTimestep expects.  Ranks beyond
    // AttributeBlocks.size() get a 0-sized "block" the reader will skip.
    const size_t writerCount = static_cast<size_t>(m_Comm.Size());
    while (AttrSizeVector.size() < writerCount)
    {
        AttrSizeVector.push_back(0);
        MDataTotalSize += sizeof(uint64_t);
    }

    m_MetadataFile->Write((char *)&MDataTotalSize, sizeof(uint64_t));
    MetaDataSize += sizeof(uint64_t);
    m_MetadataFile->Write((char *)AttrSizeVector.data(), sizeof(uint64_t) * AttrSizeVector.size());
    MetaDataSize += sizeof(uint64_t) * AttrSizeVector.size();

    for (auto &b : AttributeBlocks)
    {
        if (!b.iov_base)
            continue;
        m_MetadataFile->Write((char *)b.iov_base, b.iov_len);
        MetaDataSize += b.iov_len;
    }

    m_MetaDataPos += MetaDataSize;
    return MetaDataSize;
}

void DaosWriter::WriteData(format::BufferV *Data)
{
    // DaosChunkV has been submitting async writes during AddToVec.
    // Drain pending events, then write any tail (still-filling chunk +
    // any externs that raced) synchronously to the per-rank array.
    auto *daosBuf = static_cast<format::DaosChunkV *>(Data);
    {
        profiling::ProfilerGuard g(m_Profiler, "WD_Drain");
        daosBuf->Drain();
    }

    auto tail = daosBuf->UnflushedTail();
    if (!tail.empty())
    {
        profiling::ProfilerGuard g(m_Profiler, "WD_Tail");
        // One synchronous daos_array_write covering the unflushed tail
        // with one iov per VecEntry in the SGL.  Same shape as
        // DaosChunkV's async submits, just synchronous and last.
        daos_size_t total = 0;
        std::vector<d_iov_t> sgiovs;
        sgiovs.reserve(tail.size());
        for (const auto &e : tail)
        {
            if (e.iov_len == 0)
                continue;
            d_iov_t v;
            v.iov_buf = const_cast<void *>(e.iov_base);
            v.iov_buf_len = e.iov_len;
            v.iov_len = e.iov_len;
            sgiovs.push_back(v);
            total += e.iov_len;
        }
        if (total > 0)
        {
            daos_range_t rg;
            rg.rg_idx = m_DataArrayCursor + daosBuf->SubmittedHigh();
            rg.rg_len = total;
            daos_array_iod_t iod;
            iod.arr_nr = 1;
            iod.arr_rgs = &rg;
            d_sg_list_t sgl;
            sgl.sg_nr = static_cast<uint32_t>(sgiovs.size());
            sgl.sg_nr_out = 0;
            sgl.sg_iovs = sgiovs.data();
            int rc = daos_array_write(m_DataArrayOH, DAOS_TX_NONE, &iod, &sgl, NULL);
            if (rc)
            {
                helper::Throw<std::runtime_error>("Engine", "DaosWriter", "WriteData",
                                                  "tail daos_array_write failed rc=" +
                                                      std::to_string(rc));
            }
        }
    }

    m_Profiler.AddBytes("totalBytes", Data->Size());
    m_DataArrayCursor += Data->Size();
    // DON'T delete Data: with chunk recycling, Data == m_DataBuf.get()
    // and is engine-lifetime.  Next BeginStep calls Reset() and reuses it.
}

void DaosWriter::WriteMetadataFileIndex(uint64_t MetaDataPos, uint64_t MetaDataSize)
{
    profiling::ProfilerGuard g(m_Profiler, "MD_Posix");
    m_MetadataFile->Flush();

    const uint64_t writerCount = static_cast<uint64_t>(m_Comm.Size());

    // Phase-1 StepRecord layout: only (MetaDataPos, MetaDataSize).  Each
    // rank's step-start data offset rides on its own metadata-blob
    // trailer in DAOS, so md.idx no longer carries any per-writer block.
    //
    // First-call layout:
    //   header (m_IndexHeaderSize bytes)
    //   WriterMapRecord (1 + 8 + (3 + writerCount) * 8 bytes)
    //   StepRecord      (1 + 8 + 16 bytes)
    // Subsequent calls: just StepRecord.
    //
    // The WriterMapRecord still populates the reader's m_WriterMap so
    // m_WriterMap[step].WriterCount is non-zero — without this,
    // DaosArrayReadMetadata's daos_kv_get for the per-step metadata
    // sizes is called with writer_count=0 and deadlocks.  The subfile
    // indices in the map are nominal (every rank "owns" its own
    // subfile slot); the actual data is reached via data_oids.txt.
    const size_t stepBodyBytes = 2 * sizeof(uint64_t);
    size_t bufsize = 1 + sizeof(uint64_t) + stepBodyBytes;
    const bool isFirstCall = m_FirstIndexCall;
    if (isFirstCall)
    {
        bufsize += m_IndexHeaderSize;
        bufsize += 1 + sizeof(uint64_t) + (3 + writerCount) * sizeof(uint64_t);
    }

    std::vector<char> buf(bufsize);
    size_t pos = 0;
    uint64_t d;
    unsigned char record;

    if (isFirstCall)
    {
        MakeHeader(buf, pos, "Index Table", true);

        // WriterMapRecord
        record = WriterMapRecord;
        helper::CopyToBuffer(buf, pos, &record, 1);
        d = (3 + writerCount) * sizeof(uint64_t);
        helper::CopyToBuffer(buf, pos, &d, 1);           // record length
        helper::CopyToBuffer(buf, pos, &writerCount, 1); // WriterCount
        helper::CopyToBuffer(buf, pos, &writerCount, 1); // AggregatorCount = N
        helper::CopyToBuffer(buf, pos, &writerCount, 1); // SubfileCount = N
        for (uint64_t i = 0; i < writerCount; ++i)
        {
            helper::CopyToBuffer(buf, pos, &i, 1); // RankToSubfile[i] = i
        }
        m_FirstIndexCall = false;
    }

    // StepRecord
    record = StepRecord;
    helper::CopyToBuffer(buf, pos, &record, 1);
    d = stepBodyBytes;
    helper::CopyToBuffer(buf, pos, &d, 1); // record length
    helper::CopyToBuffer(buf, pos, &MetaDataPos, 1);
    helper::CopyToBuffer(buf, pos, &MetaDataSize, 1);

    m_MetadataIndexFile->Write((char *)buf.data(), buf.size());
}

void DaosWriter::NotifyEngineAttribute(std::string name, DataType type) noexcept
{
    helper::Throw<std::invalid_argument>("DaosWriter", "Engine", "ThrowUp",
                                         "Engine does not support NotifyEngineAttribute");
}

void DaosWriter::NotifyEngineAttribute(std::string name, AttributeBase *Attr, void *data) noexcept
{
    if (!m_Parameters.UseOneTimeAttributes)
    {
        m_MarshalAttributesNecessary = true;
        return;
    }

    m_BP5Serializer.OnetimeMarshalAttribute(*Attr);
    m_MarshalAttributesNecessary = false;
}

void DaosWriter::MarshalAttributes()
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    const auto &attributes = m_IO.GetAttributes();

    // if there are no new attributes, nothing to do
    if (!m_MarshalAttributesNecessary)
    {
        return;
    }
    m_MarshalAttributesNecessary = false;

    for (const auto &attributePair : attributes)
    {
        const std::string name(attributePair.first);
        auto baseAttr = &attributePair.second;
        const DataType type((*baseAttr)->m_Type);
        int element_count = -1;

        if (!attributePair.second->m_IsSingleValue)
        {
            element_count = (*baseAttr)->m_Elements;
        }

        if (type == DataType::None)
        {
        }
        else if (type == helper::GetDataType<std::string>())
        {
            core::Attribute<std::string> &attribute = *m_IO.InquireAttribute<std::string>(name);
            void *data_addr;
            if (attribute.m_IsSingleValue)
            {
                data_addr = (void *)attribute.m_DataSingleValue.c_str();
            }
            else
            {
                const char **tmp = (const char **)malloc(sizeof(char *) * element_count);
                for (int i = 0; i < element_count; i++)
                {
                    auto str = &attribute.m_DataArray[i];
                    tmp[i] = str->c_str();
                }
                // tmp will be free'd after final attribute marshalling
                data_addr = (void *)tmp;
            }

            m_BP5Serializer.MarshalAttribute(name.c_str(), type, sizeof(char *), element_count,
                                             data_addr);
        }
#define declare_type(T)                                                                            \
    else if (type == helper::GetDataType<T>())                                                     \
    {                                                                                              \
        core::Attribute<T> &attribute = *m_IO.InquireAttribute<T>(name);                           \
        int element_count = -1;                                                                    \
        void *data_addr = &attribute.m_DataSingleValue;                                            \
        if (!attribute.m_IsSingleValue)                                                            \
        {                                                                                          \
            element_count = attribute.m_Elements;                                                  \
            data_addr = attribute.m_DataArray.data();                                              \
        }                                                                                          \
        m_BP5Serializer.MarshalAttribute(attribute.m_Name.c_str(), type, sizeof(T), element_count, \
                                         data_addr);                                               \
    }

        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

std::vector<char> DaosWriter::BuildMetadataBlob(format::BP5Serializer::TimestepInfo &TSInfo)
{
    const size_t metaSize = TSInfo.MetaEncodeBuffer ? TSInfo.MetaEncodeBuffer->m_FixedSize : 0;

    if (!m_PerRankMetadata)
    {
        // Aggregated mode (Phase 1): [MetaEncode][step_start u64].
        std::vector<char> buf(metaSize + sizeof(uint64_t));
        if (metaSize > 0)
            std::memcpy(buf.data(), TSInfo.MetaEncodeBuffer->Data(), metaSize);
        std::memcpy(buf.data() + metaSize, &m_StepStartDataPos, sizeof(uint64_t));
        return buf;
    }

    // Per-rank mode (Phase 2): pack MetaEncode + MetaMetaBlocks +
    // AttributeEncodeBuffer + 24-byte footer.  Footer is
    // (mmb_size, attr_size, step_start) — sizes count only the
    // serialized body bytes, not the footer itself.
    const auto &mmbs = TSInfo.NewMetaMetaBlocks;
    uint64_t mmbBodySize = sizeof(uint64_t); // count prefix
    for (const auto &m : mmbs)
        mmbBodySize += 2 * sizeof(uint64_t) + m.MetaMetaIDLen + m.MetaMetaInfoLen;

    const uint64_t attrSize =
        (TSInfo.AttributeEncodeBuffer && TSInfo.AttributeEncodeBuffer->m_FixedSize > 0)
            ? static_cast<uint64_t>(TSInfo.AttributeEncodeBuffer->m_FixedSize)
            : 0;
    const size_t footerSize = 3 * sizeof(uint64_t);
    const size_t total = metaSize + mmbBodySize + attrSize + footerSize;

    std::vector<char> buf(total);
    size_t pos = 0;
    if (metaSize > 0)
    {
        std::memcpy(buf.data() + pos, TSInfo.MetaEncodeBuffer->Data(), metaSize);
        pos += metaSize;
    }
    // MetaMetaBlocks body: count + (idLen, infoLen, id, info)+
    const uint64_t mmbCount = static_cast<uint64_t>(mmbs.size());
    std::memcpy(buf.data() + pos, &mmbCount, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    for (const auto &m : mmbs)
    {
        const uint64_t idLen = m.MetaMetaIDLen;
        const uint64_t infoLen = m.MetaMetaInfoLen;
        std::memcpy(buf.data() + pos, &idLen, sizeof(uint64_t));
        pos += sizeof(uint64_t);
        std::memcpy(buf.data() + pos, &infoLen, sizeof(uint64_t));
        pos += sizeof(uint64_t);
        std::memcpy(buf.data() + pos, m.MetaMetaID, idLen);
        pos += idLen;
        std::memcpy(buf.data() + pos, m.MetaMetaInfo, infoLen);
        pos += infoLen;
    }
    // AttributeEncodeBuffer
    if (attrSize > 0)
    {
        std::memcpy(buf.data() + pos, TSInfo.AttributeEncodeBuffer->Data(), attrSize);
        pos += attrSize;
    }
    // Footer
    std::memcpy(buf.data() + pos, &mmbBodySize, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    std::memcpy(buf.data() + pos, &attrSize, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    std::memcpy(buf.data() + pos, &m_StepStartDataPos, sizeof(uint64_t));
    pos += sizeof(uint64_t);
    return buf;
}

void DaosWriter::DaosArrayWriteMetadata(format::BP5Serializer::TimestepInfo &TSInfo)
{
    /* Allgather metadata sizes from all processes; absorbs cross-rank
       skew left by the no-Barrier-after-SelAgg decision in EndStep. */
    std::vector<char> blob = BuildMetadataBlob(TSInfo);
    const uint64_t myMetaTotal = static_cast<uint64_t>(blob.size());
    std::vector<uint64_t> list_metadata_size(m_Comm.Size());
    m_Comm.Allgather(&myMetaTotal, 1, list_metadata_size.data(), 1);

    size_t offset = 0;
    if (metadataLayout == MetadataLayout::ARRAY)
    {
        for (int i = 0; i < m_Comm.Rank(); i++)
            offset += list_metadata_size[i];
    }
    else if (metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {
        // 64-bit multiply: chunk_size_1mb and Comm::Rank() are both int, so
        // an int*int product overflows at rank 2048 (2048 * 1 MiB == 2^31).
        // The reader computes this offset in size_t and reads the correct
        // location, so an overflow here silently strands every blob from
        // rank 2048 on.  Cast the rank to widen the whole expression.
        offset = static_cast<size_t>(m_Comm.Rank()) * chunk_size_1mb;
    }

    // Setup I/O Descriptor
    iod.arr_nr = 1;
    rg.rg_len = myMetaTotal;
    rg.rg_idx = m_step_offset + offset;
    iod.arr_rgs = &rg;

    /** memory location: single contiguous blob */
    d_iov_set(&iov, blob.data(), myMetaTotal);
    sgl.sg_nr = 1;
    sgl.sg_iovs = &iov;

    // Write Metadata
    CALI_MARK_BEGIN("DaosWriter::daos_array_write");
    int rc = daos_array_write(oh, DAOS_TX_NONE, &iod, &sgl, NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "DaosArrayWriteMetadata",
                                          "daos_array_write failed rc=" + std::to_string(rc));
    }
    CALI_MARK_END("DaosWriter::daos_array_write");

    m_step_offset += MAX_AGGREGATE_METADATA_SIZE;

    // Writer Rank 0 -Store the list of metadata size in a KV entry
    if (m_Comm.Rank() == 0)
    {
        char key[1000];
        sprintf(key, "step%zu", m_WriterStep);
        CALI_MARK_BEGIN("DaosWriter::daos_kv_put");
        rc = daos_kv_put(mdsize_oh, DAOS_TX_NONE, 0, key,
                         sizeof(uint64_t) * list_metadata_size.size(), list_metadata_size.data(),
                         NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "DaosArrayWriteMetadata",
                                              "daos_kv_put (mdsize) failed rc=" +
                                                  std::to_string(rc));
        }
        CALI_MARK_END("DaosWriter::daos_kv_put");
    }
}

void DaosWriter::DaosKVWriteMetadata(format::BP5Serializer::TimestepInfo &TSInfo)
{
    char key[1000];
    int rc;
    sprintf(key, "step%zu-rank%d", m_WriterStep, m_Comm.Rank());

    // Value = composite blob (see BuildMetadataBlob).  Either Phase-1
    // (MetaEncode + 8-byte trailer) or Phase-2 (MetaEncode + MetaMeta
    // + Attr + 24-byte footer) depending on m_PerRankMetadata.
    std::vector<char> blob = BuildMetadataBlob(TSInfo);

    CALI_MARK_BEGIN("DaosWriter::daos_kv_put");
    rc = daos_kv_put(oh, DAOS_TX_NONE, 0, key, blob.size(), blob.data(), NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "DaosKVWriteMetadata",
                                          "daos_kv_put failed rc=" + std::to_string(rc));
    }
    CALI_MARK_END("DaosWriter::daos_kv_put");
}

void DaosWriter::WriteMetadata(format::BP5Serializer::TimestepInfo &TSInfo)
{
    profiling::ProfilerGuard g(m_Profiler, "MD_Daos");
    switch (metadataLayout)
    {
    case MetadataLayout::ARRAY:
    case MetadataLayout::ARRAY_1MB_ALIGNED:
        DaosArrayWriteMetadata(TSInfo);
        break;
    case MetadataLayout::KV:
        DaosKVWriteMetadata(TSInfo);
        break;
    }
}

void DaosWriter::EndStep()
{
    /* Seconds ts = Now() - m_EngineStart;
      std::cout << "END STEP starts at: " << ts.count() << std::endl; */
    m_BetweenStepPairs = false;
    PERFSTUBS_SCOPED_TIMER("DaosWriter::EndStep");
    profiling::ProfilerGuard es(m_Profiler, "ES");

    m_Profiler.Start("ES_CloseTS");
    MarshalAttributes();

    // true: advances step
    auto TSInfo = m_BP5Serializer.CloseTimestep(m_WriterStep, m_Parameters.AsyncWrite);
    m_ThisTimestepDataSize += TSInfo.DataBuffer->Size();
    m_Profiler.Stop("ES_CloseTS");

    {
        profiling::ProfilerGuard wd(m_Profiler, "ES_WriteData");
        adios2::format::BufferV *databuf = TSInfo.DataBuffer;
        TSInfo.DataBuffer = nullptr;
        CALI_MARK_BEGIN("DaosWriter::WriteData");
        WriteData(databuf);
        CALI_MARK_END("DaosWriter::WriteData");
    }

    /*
     * BP5-style selective metadata aggregation.
     *
     * Cheap fixed-size gather + bcast that reaches collective consensus
     * on which MetaMetaBlocks and Attribute blocks are unique and need
     * to be written by rank 0.  After step 0 in stable HPC apps, the
     * bitmap is empty almost every step and rank 0 has no writes —
     * eliminating the per-step coordination cost.  Per-variable
     * metadata is unaffected (it goes per-rank to DAOS via WriteMetadata
     * below, outside this block).  No trailing m_Comm.Barrier — the
     * Allgather inside DaosArrayWriteMetadata absorbs skew.
     *
     * Rank 0 also emits a step record to md.idx every step (even when
     * the gate says no new content) — the reader's attribute path
     * uses m_MetadataIndexTable[Step][2] to locate attributes in md.0,
     * so an entry per step is required for attribute reads to work.
     *
     * Cheap collective gate: blocking Allreduce of a 1-byte "anyone has
     * new content?" flag.  Aurora's MPI doesn't auto-progress
     * non-blocking collectives, so an MPI_Iallreduce + Wait version
     * offered no improvement (verified empirically).  Keep it blocking;
     * the cost is ~80-100 ms/step at 32 ranks, which is the floor for
     * one MPI collective at scale on this network.
     */
    std::vector<core::iovec> AttributeBlocks;
    if (!m_PerRankMetadata)
    {
        // ---- aggregated mode (Phase 1 / default) ----
        unsigned char localHasContent =
            (!TSInfo.NewMetaMetaBlocks.empty() ||
             (TSInfo.AttributeEncodeBuffer && TSInfo.AttributeEncodeBuffer->m_FixedSize > 0))
                ? 1
                : 0;
        unsigned char anyHasContent = 0;
        m_Profiler.Start("ES_Gate");
        m_Comm.Allreduce(&localHasContent, &anyHasContent, 1, helper::Comm::Op::Max);
        m_Profiler.Stop("ES_Gate");

        if (anyHasContent)
        {
            profiling::ProfilerGuard g(m_Profiler, "ES_SelAgg");

            std::vector<format::BP5Base::MetaMetaInfoBlock> UniqueMetaMetaBlocks =
                TSInfo.NewMetaMetaBlocks;
            if (TSInfo.AttributeEncodeBuffer)
            {
                AttributeBlocks.push_back({TSInfo.AttributeEncodeBuffer->Data(),
                                           TSInfo.AttributeEncodeBuffer->m_FixedSize});
            }
            std::vector<size_t> MetaEncodeSize{
                TSInfo.MetaEncodeBuffer ? TSInfo.MetaEncodeBuffer->m_FixedSize : 0};
            // Aggregated WriterDataPositions output is no longer used —
            // each rank carries its own step-start position in its
            // metadata trailer.  Pass m_StepStartDataPos as input only
            // because BP5Helper's fixed-node-contrib packet requires it.
            std::vector<uint64_t> WriterDataPos{m_StepStartDataPos};

            format::BP5Helper::BP5AggregateInformation(m_Comm, m_Profiler, UniqueMetaMetaBlocks,
                                                       AttributeBlocks, MetaEncodeSize,
                                                       WriterDataPos);

            if (m_Comm.Rank() == 0 && !UniqueMetaMetaBlocks.empty())
            {
                WriteMetaMetadata(UniqueMetaMetaBlocks);
                for (auto &mm : UniqueMetaMetaBlocks)
                {
                    free((void *)mm.MetaMetaInfo);
                    free((void *)mm.MetaMetaID);
                }
            }
            // No trailing Barrier — the next collective is the Allgather
            // inside DaosArrayWriteMetadata, which absorbs cross-rank skew.
        }

        // Rank 0 always emits an attribute record to md.0 (even when empty —
        // WriteAttributes pads AttrSizeVector to writerCount zeros).
        if (m_Comm.Rank() == 0)
        {
            m_LatestMetaDataPos = m_MetaDataPos;
            m_LatestMetaDataSize = WriteAttributes(AttributeBlocks);
            WriteMetadataFileIndex(m_LatestMetaDataPos, m_LatestMetaDataSize);
        }
    }
    else
    {
        // ---- per-rank mode (Phase 2) ----
        // No ES_Gate, no ES_SelAgg.  Each rank's NewMetaMetaBlocks and
        // AttributeEncodeBuffer are serialized into its own metadata
        // blob inside WriteMetadata below.  Rank 0 still emits a step
        // record to md.idx (degenerate: MetaDataPos/Size = 0) so the
        // reader's step-count machinery still works.  mmd.0 / md.0
        // stay empty in this mode.
        if (m_Comm.Rank() == 0)
        {
            WriteMetadataFileIndex(0, 0);
        }
    }

    CALI_MARK_BEGIN("DaosWriter::metadata-stabilization");
    WriteMetadata(TSInfo);
    CALI_MARK_END("DaosWriter::metadata-stabilization");

    m_WriterStep++;
    // Open the BetweenSteps timer; the next BeginStep closes it.  This
    // attributes iotest's data-generation gap (or whatever the workload
    // does between EndStep return and the next BeginStep call) to a
    // named timer so it can be subtracted when comparing against the
    // bench, which has no equivalent gap.
    m_Profiler.Start("BetweenSteps");
}

// PRIVATE
void DaosWriter::Init()
{
    if (m_IO.m_TransportsParameters.size() > 1)
    {
        helper::Throw<std::invalid_argument>(
            "Engine", "DaosWriter", "Init",
            "DAOS engine accepts only one transport per IO; got " +
                std::to_string(m_IO.m_TransportsParameters.size()) +
                ". Multiple parallel transports per IO are no longer supported.");
    }

    m_BP5Serializer.m_Engine = this;
    m_RankMPI = m_Comm.Rank();
    InitParameters();
    InitTransports();
    CALI_MARK_BEGIN("DaosWriter::InitDAOS");
    InitDAOS();
    CALI_MARK_END("DaosWriter::InitDAOS");

    RegisterProfilerTimers();
    OpenDataArray();

    InitBPBuffer();
}

void DaosWriter::RegisterProfilerTimers()
{
    // Standard BP5-style timers (BS, ES, ES_CloseTS, ES_WriteData, PP)
    // are pre-registered by the JSONProfiler ctor; only the DAOS-engine-
    // specific names need to be added here.  Names are reused at every
    // ProfilerGuard call site and as keys in profiling.json.
    m_Profiler.AddTimerWatch("OpenSetup");
    m_Profiler.AddTimerWatch("PersistOids");
    m_Profiler.AddTimerWatch("WD_Drain");
    m_Profiler.AddTimerWatch("WD_Tail");
    m_Profiler.AddTimerWatch("MD_Daos");
    m_Profiler.AddTimerWatch("MD_Posix");
    m_Profiler.AddTimerWatch("PutCommon");
    m_Profiler.AddTimerWatch("PutCommon_Marshal");
    m_Profiler.AddTimerWatch("ES_Gate");
    m_Profiler.AddTimerWatch("ES_SelAgg");
    m_Profiler.AddTimerWatch("BetweenSteps");
    // DaosChunkV sub-timers (called via the profiler reference passed
    // to its constructor).
    m_Profiler.AddTimerWatch("atvMemcpy");
    m_Profiler.AddTimerWatch("atvReap");
    m_Profiler.AddTimerWatch("atvSubmit");
    m_Profiler.AddTimerWatch("subSetup");
    m_Profiler.AddTimerWatch("subEventInit");
    m_Profiler.AddTimerWatch("subWriteCall");
    // Mirrors of accumulators that live outside m_Profiler
    // (BP5Serializer's per-call sub-Marshal counters and DaosChunkV's
    // fresh-allocation count).  Values are copied in at DoClose by
    // SyncExternalCountersToProfiler so they appear in profiling.json.
    m_Profiler.AddTimerWatch("BP5_BufferAppend");
    m_Profiler.AddTimerWatch("BP5_GetMinMax");
    m_Profiler.AddTimerWatch("FreshChunkAllocs");
    m_Profiler.AddTimerWatch("DAOS_ZeroCopyPuts");
    m_Profiler.AddTimerWatch("DAOS_ZeroCopyMiB");
    m_Profiler.AddTimerWatch("DAOS_CopyPuts");
    m_Profiler.AddTimerWatch("DAOS_CopyMiB");
}

void DaosWriter::SyncExternalCountersToProfiler()
{
    auto setTimer = [&](const std::string &name, double secs, uint64_t calls) {
        auto *t = m_Profiler.GetTimer(name);
        if (t)
        {
            t->m_ProcessTime = static_cast<int64_t>(secs * 1.0e6);
            t->m_nCalls = calls;
        }
    };

    setTimer("BP5_BufferAppend", m_BP5Serializer.m_BufferAppendSecs,
             static_cast<uint64_t>(m_BP5Serializer.m_BufferAppendCalls));
    // m_GetMinMaxSecs has no call counter; use a sentinel so the
    // timer entry is emitted whenever any time was accumulated.
    setTimer("BP5_GetMinMax", m_BP5Serializer.m_GetMinMaxSecs,
             m_BP5Serializer.m_GetMinMaxSecs > 0 ? 1 : 0);
    setTimer("FreshChunkAllocs", 0.0,
             m_DataBuf ? static_cast<uint64_t>(m_DataBuf->FreshAllocCount()) : 0);
    setTimer("DAOS_ZeroCopyPuts", 0.0, m_ZeroCopyPuts);
    setTimer("DAOS_ZeroCopyMiB", 0.0, m_ZeroCopyBytes >> 20);
    setTimer("DAOS_CopyPuts", 0.0, m_CopyPuts);
    setTimer("DAOS_CopyMiB", 0.0, m_CopyBytes >> 20);
}

void DaosWriter::InitParameters()
{
    ParseParams(m_IO, m_Parameters);
    m_BP5Serializer.m_StatsLevel = m_Parameters.StatsLevel;

    if (const char *ww = std::getenv("DAOS_WARMTH_WINDOW"))
    {
        m_WarmthWindow = static_cast<size_t>(std::strtoul(ww, nullptr, 10));
    }
}

void DaosWriter::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    // Compute paths for the BP5 metadata files (md.0, mmd.0, md.idx) and
    // the DAOS-engine OID file.  No data subfiles: data lives in per-rank
    // DAOS arrays located via data_oids.txt (written separately by
    // PersistDataOidsIndex).
    const std::vector<std::string> transportsNames =
        transportman::TransportMan::GetFilesBaseNames(m_Name, m_IO.m_TransportsParameters);

    if (m_Comm.Rank() == 0)
    {
        m_MetadataFileNames = GetBPMetadataFileNames(transportsNames);
        m_MetaMetadataFileNames = GetBPMetaMetadataFileNames(transportsNames);
        m_MetadataIndexFileNames = GetBPMetadataIndexFileNames(transportsNames);
        m_OIDFileName = GetOIDFileName(transportsNames[0]);
    }
    transport::MkDirsBarrier(m_Comm, m_MetadataFileNames, m_IO.m_TransportsParameters,
                             m_Parameters.NodeLocal);

    if (m_Comm.Rank() == 0)
    {
        m_MetaMetadataFile = transport::OpenFile(m_Comm, m_MetaMetadataFileNames[0], m_OpenMode,
                                                 m_IO.m_TransportsParameters[0], true);
        m_MetadataFile = transport::OpenFile(m_Comm, m_MetadataFileNames[0], m_OpenMode,
                                             m_IO.m_TransportsParameters[0], true);
        m_MetadataIndexFile = transport::OpenFile(m_Comm, m_MetadataIndexFileNames[0], m_OpenMode,
                                                  m_IO.m_TransportsParameters[0], true);
    }
}

/// Pick the metadata-layout from the DAOS_METADATA_LAYOUT environment
/// variable.  Defaults to ARRAY_1MB_ALIGNED when unset.  An unrecognized
/// value is rejected so a typo doesn't silently fall back.
void DaosWriter::SetMetadataLayout()
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
        helper::Throw<std::invalid_argument>("Engine", "DaosWriter", "SetMetadataLayout",
                                             "DAOS_METADATA_LAYOUT='" + layoutStr +
                                                 "' is not recognized; expected one of "
                                                 "'array', 'array-1mb-aligned', 'kv'");
    }
}

/// Pick the per-rank-metadata mode from DAOS_PER_RANK_METADATA.  When
/// unset or 0, the Phase-1 default (aggregated mmd / attrs via ES_Gate
/// + ES_SelAgg) runs.  When 1, each rank serializes its own
/// NewMetaMetaBlocks + AttributeEncodeBuffer into its metadata blob.
void DaosWriter::SetPerRankMetadata()
{
    const char *env = std::getenv("DAOS_PER_RANK_METADATA");
    if (!env)
    {
        m_PerRankMetadata = false;
        return;
    }
    std::string v(env);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    if (v == "0" || v == "false" || v == "off")
    {
        m_PerRankMetadata = false;
    }
    else if (v == "1" || v == "true" || v == "on")
    {
        m_PerRankMetadata = true;
    }
    else
    {
        helper::Throw<std::invalid_argument>("Engine", "DaosWriter", "SetPerRankMetadata",
                                             "DAOS_PER_RANK_METADATA='" + v +
                                                 "' is not recognized; expected 0/1");
    }
}

// Set m_PoolName and m_ContName from the environment variables DAOS_POOL and DAOS_CONT
void DaosWriter::SetPoolAndContName()
{
    const char *pool = std::getenv("DAOS_POOL");
    const char *cont = std::getenv("DAOS_CONT");
    if (!pool || !cont)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "SetPoolAndContName",
            "DAOS_POOL and DAOS_CONT environment variables must both be set");
    }

    strncpy(m_pool_label, pool, sizeof(m_pool_label) - 1);
    m_pool_label[sizeof(m_pool_label) - 1] = '\0';
    strncpy(m_cont_label, cont, sizeof(m_cont_label) - 1);
    m_cont_label[sizeof(m_cont_label) - 1] = '\0';
}

void DaosWriter::InitDAOS()
{
    // Rank 0 - Connect to DAOS pool, and open container
    int rc;
    CALI_MARK_BEGIN("DaosWriter::daos_init");
    rc = daos_init();
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "InitDAOS",
                                          "daos_init failed rc=" + std::to_string(rc));
    }
    CALI_MARK_END("DaosWriter::daos_init");

    rc = gethostname(node, sizeof(node));
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "InitDAOS",
                                          "gethostname failed (buffer too small)");
    }

    SetMetadataLayout();
    SetPerRankMetadata();
    SetPoolAndContName();

    CALI_MARK_BEGIN("DaosWriter::daos_pool_connect");
    if (m_Comm.Rank() == 0)
    {
        /** connect to the just created DAOS pool */
        rc = daos_pool_connect(m_pool_label, DSS_PSETID, DAOS_PC_RW /* read write access */,
                               &poh /* returned pool handle */, NULL /* returned pool info */,
                               NULL /* event */);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "InitDAOS",
                                              "daos_pool_connect failed rc=" + std::to_string(rc));
        }
    }
    CALI_MARK_END("DaosWriter::daos_pool_connect");

    /** share pool handle with peer tasks */
    CALI_MARK_BEGIN("DaosWriter::daos_handle_share_pool");
    daos_handle_share(&poh, DaosWriter::HANDLE_POOL);
    CALI_MARK_END("DaosWriter::daos_handle_share_pool");

    CALI_MARK_BEGIN("DaosWriter::daos_cont_open");
    if (m_Comm.Rank() == 0)
    {
        /** open container */
        rc = daos_cont_open(poh, m_cont_label, DAOS_COO_RW, &coh, NULL, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "InitDAOS",
                                              "daos_cont_open failed rc=" + std::to_string(rc));
        }
    }
    CALI_MARK_END("DaosWriter::daos_cont_open");

    /** share container handle with peer tasks */
    CALI_MARK_BEGIN("DaosWriter::daos_handle_share_cont");
    daos_handle_share(&coh, HANDLE_CO);
    CALI_MARK_END("DaosWriter::daos_handle_share_cont");

    if (m_Comm.Rank() == 0)
    {
        switch (metadataLayout)
        {
        case MetadataLayout::ARRAY:
        case MetadataLayout::ARRAY_1MB_ALIGNED:
            CreateDaosArrayObject();
            break;
        case MetadataLayout::KV:
            CreateDaosKVObject();
            break;
        }
    }

    OpenDaosObjAndShare();

    if (m_Comm.Rank() == 0)
        WriteObjectIDsToFile();
}

void DaosWriter::WriteObjectIDsToFile()
{
    FILE *fp = fopen(m_OIDFileName.c_str(), "w");
    if (fp == NULL)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "WriteObjectIDsToFile",
                                          "fopen failed for " + m_OIDFileName + ": " +
                                              std::string(strerror(errno)));
    }
    if (metadataLayout == MetadataLayout::ARRAY ||
        metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {

        fprintf(fp, "%" PRIu64 "\n%" PRIu64 "\n", oid.hi, oid.lo);
        fprintf(fp, "%" PRIu64 "\n%" PRIu64 "\n", mdsize_oid.hi, mdsize_oid.lo);
        fclose(fp);
    }
    else if (metadataLayout == MetadataLayout::KV)
    {
        fprintf(fp, "%" PRIu64 "\n%" PRIu64 "\n", oid.hi, oid.lo);
        fclose(fp);
    }
}

void DaosWriter::OpenDaosObjAndShare()
{
    if (metadataLayout == MetadataLayout::ARRAY ||
        metadataLayout == MetadataLayout::ARRAY_1MB_ALIGNED)
    {
        /** share array object handle with peer tasks */
        CALI_MARK_BEGIN("DaosWriter::array_oh_share");
        array_oh_share(&oh);
        CALI_MARK_END("DaosWriter::array_oh_share");
    }
    else if (metadataLayout == MetadataLayout::KV)
    {
        m_Comm.Bcast((char *)&oid, sizeof(daos_obj_id_t), 0);
        // Open KV object
        CALI_MARK_BEGIN("DaosWriter::daos_kv_open");
        int rc = daos_kv_open(coh, oid, DAOS_OO_RW, &oh, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "OpenDaosObjAndShare",
                                              "daos_kv_open failed rc=" + std::to_string(rc));
        }
        CALI_MARK_END("DaosWriter::daos_kv_open");
    }
}

void DaosWriter::array_oh_share(daos_handle_t *oh)
{
    d_iov_t ghdl = {NULL, 0, 0};
    int rc;

    if (m_Comm.Rank() == 0)
    {
        /** fetch size of global handle */
        rc = daos_array_local2global(*oh, &ghdl);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "array_oh_share",
                                              "daos_array_local2global (size) failed rc=" +
                                                  std::to_string(rc));
        }
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
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "array_oh_share",
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
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "array_oh_share",
                                              "daos_array_global2local failed rc=" +
                                                  std::to_string(rc));
        }
    }

    free(ghdl.iov_buf);

    m_Comm.Barrier();
}

/*generate the header for the metadata index file*/
void DaosWriter::MakeHeader(std::vector<char> &buffer, size_t &position, const std::string fileType,
                            const bool isActive)
{
    auto lf_CopyVersionChar = [](const std::string version, std::vector<char> &buffer,
                                 size_t &position) {
        helper::CopyToBuffer(buffer, position, version.c_str());
    };

    // auto &buffer = b.m_Buffer;
    // auto &position = b.m_Position;
    // auto &absolutePosition = b.m_AbsolutePosition;
    if (position > 0)
    {
        helper::Throw<std::invalid_argument>(
            "Engine", "DaosWriter", "MakeHeader",
            "BP4Serializer::MakeHeader can only be called for an empty "
            "buffer. This one for " +
                fileType + " already has content of " + std::to_string(position) + " bytes.");
    }

    if (buffer.size() < m_IndexHeaderSize)
    {
        buffer.resize(m_IndexHeaderSize);
    }

    const std::string majorVersion(std::to_string(ADIOS2_VERSION_MAJOR));
    const std::string minorVersion(std::to_string(ADIOS2_VERSION_MINOR));
    const std::string patchVersion(std::to_string(ADIOS2_VERSION_PATCH));

    // byte 0-31: Readable tag
    if (position != m_VersionTagPosition)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "MakeHeader",
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Version Tag "
            "position mismatch");
    }
    std::string versionLongTag("ADIOS-BP v" + majorVersion + "." + minorVersion + "." +
                               patchVersion + " ");
    size_t maxTypeLen = m_VersionTagLength - versionLongTag.size();
    const std::string fileTypeStr = fileType.substr(0, maxTypeLen);
    versionLongTag += fileTypeStr;
    const size_t versionLongTagSize = versionLongTag.size();
    if (versionLongTagSize < m_VersionTagLength)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(), versionLongTagSize);
        position += m_VersionTagLength - versionLongTagSize;
    }
    else if (versionLongTagSize > m_VersionTagLength)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(), m_VersionTagLength);
    }
    else
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(), m_VersionTagLength);
    }

    // byte 32-35: MAJOR MINOR PATCH Unused

    lf_CopyVersionChar(majorVersion, buffer, position);
    lf_CopyVersionChar(minorVersion, buffer, position);
    lf_CopyVersionChar(patchVersion, buffer, position);
    ++position;

    // Note: Reader does process and use bytes 36-38 in
    // BP4Deserialize.cpp::ParseMetadataIndex().
    // Order and position must match there.

    // byte 36: endianness
    if (position != m_EndianFlagPosition)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "MakeHeader",
            "ADIOS Coding ERROR in DaosWriter::MakeHeader. Endian Flag "
            "position mismatch");
    }
    const uint8_t endianness = helper::IsLittleEndian() ? 0 : 1;
    helper::CopyToBuffer(buffer, position, &endianness);

    // byte 37: BP Version 5
    if (position != m_BPVersionPosition)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "MakeHeader",
            "ADIOS Coding ERROR in DaosWriter::MakeHeader. BP Version "
            "position mismatch");
    }
    const uint8_t version = 5;
    helper::CopyToBuffer(buffer, position, &version);

    // byte 38: BP Minor version 1
    if (position != m_BPMinorVersionPosition)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "MakeHeader",
            "ADIOS Coding ERROR in DaosWriter::MakeHeader. BP Minor version "
            "position mismatch");
    }
    const uint8_t minorversion = m_BP5MinorVersion;
    helper::CopyToBuffer(buffer, position, &minorversion);

    // byte 39: Active flag (used in Index Table only)
    if (position != m_ActiveFlagPosition)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "MakeHeader",
            "ADIOS Coding ERROR in DaosWriter::MakeHeader. Active Flag "
            "position mismatch");
    }
    const uint8_t activeFlag = (isActive ? 1 : 0);
    helper::CopyToBuffer(buffer, position, &activeFlag);

    // byte 40 columnMajor
    // write if data is column major in metadata and data
    const uint8_t columnMajor = (m_IO.m_ArrayOrder == ArrayOrdering::ColumnMajor) ? 'y' : 'n';
    helper::CopyToBuffer(buffer, position, &columnMajor);

    // byte 41: per-rank-metadata mode flag (Phase 2 gating).  0 =
    // aggregated (Phase 1), 1 = per-rank attributes/MetaMetaBlocks
    // packed in each rank's blob.  Reader uses this byte to dispatch.
    if (position != m_PerRankMetadataFlagPosition)
    {
        helper::Throw<std::runtime_error>(
            "Engine", "DaosWriter", "MakeHeader",
            "ADIOS Coding ERROR in DaosWriter::MakeHeader. PerRankMetadata "
            "flag position mismatch");
    }
    const uint8_t perRankFlag = (m_PerRankMetadata ? 1 : 0);
    helper::CopyToBuffer(buffer, position, &perRankFlag);

    // byte 42-63: unused
    position += 22;
    // absolutePosition = position;
}

void DaosWriter::UpdateActiveFlag(const bool active)
{
    const char activeChar = (active ? '\1' : '\0');
    m_MetadataIndexFile->Write(&activeChar, 1, m_ActiveFlagPosition);
    m_MetadataIndexFile->Flush();
    m_MetadataIndexFile->SeekToEnd();
}

void DaosWriter::InitBPBuffer()
{
    if (m_OpenMode == Mode::Append)
    {
        helper::Throw<std::invalid_argument>("Engine", "DaosWriter", "InitBPBuffer",
                                             "Append mode is not supported by the DAOS engine "
                                             "(each Open allocates a new per-rank DAOS array)");
    }

    /* New file: prepare metadata streams.  Per-step data positions are
     * now carried in each rank's metadata blob trailer, not gathered to
     * rank 0; DaosReader locates per-rank arrays via data_oids.txt. */
    if (m_Comm.Rank() == 0)
    {
        m_MetadataIndexFile->SeekToBegin();
        m_MetadataFile->SeekToBegin();
        m_MetaMetadataFile->SeekToBegin();
    }
}

void DaosWriter::FlushData(const bool isFinal)
{
    BufferV *DataBuf = m_BP5Serializer.ReinitStepData(
        new ChunkV("DaosWriter", false, m_BP5Serializer.m_BufferAlign,
                   m_BP5Serializer.m_BufferBlockSize, m_Parameters.BufferChunkSize),
        m_Parameters.AsyncWrite);

    auto databufsize = DataBuf->Size();
    WriteData(DataBuf);
    /* DataBuf is deleted in WriteData() */
    DataBuf = nullptr;

    m_ThisTimestepDataSize += databufsize;
    // Phase-1: no per-flush (pos, size) gather.  BP5's flush-list
    // machinery is structurally dead in DAOS — each rank writes its own
    // per-rank array, so all flush segments within a step are physically
    // contiguous and the reader only needs the step-start offset (carried
    // in the per-rank metadata trailer).
}

void DaosWriter::Flush(const int transportIndex) {}

void DaosWriter::PerformDataWrite() { FlushData(false); }

void DaosWriter::DestructorClose(bool Verbose) noexcept
{
    if (Verbose)
    {
        std::cerr << "BP5 Writer \"" << m_Name << "\" Destroyed without a prior Close()."
                  << std::endl;
        std::cerr << "This may result in corrupt output." << std::endl;
    }
    // close metadata index file
    if (m_Comm.Rank() == 0)
        UpdateActiveFlag(false);
    m_IsOpen = false;
}

DaosWriter::~DaosWriter()
{
    if (m_IsOpen)
    {
        DestructorClose(m_FailVerbose);
    }
    m_IsOpen = false;
}

void DaosWriter::DoClose(const int transportIndex)
{
    PERFSTUBS_SCOPED_TIMER("DaosWriter::Close");

    if ((m_WriterStep == 0) && !m_BetweenStepPairs)
    {
        /* never did begin step, do one now */
        BeginStep(StepMode::Update);
    }
    if (m_BetweenStepPairs)
    {
        EndStep();
    }

    SyncExternalCountersToProfiler();

    CloseDataArray();

    if (m_Comm.Rank() == 0)
    {
        // close metadata files
        m_MetadataFile->Close();
        m_MetaMetadataFile->Close();

        // close metadata index file
        UpdateActiveFlag(false);
        m_MetadataIndexFile->Close();
    }

    // Release DAOS handles in reverse of init order.  Without these the
    // container/pool stay connected past Engine::Close until process exit;
    // long-running apps that open many engines in one process leaked
    // handles otherwise.  daos_fini's refcount lets libpil4dfs (if loaded)
    // continue to own its own daos_init reference safely.
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
    if (m_Comm.Rank() == 0 && poh.cookie != 0)
    {
        daos_pool_disconnect(poh, NULL);
        poh = {};
    }
    // No daos_fini() here.  We tried adding it (commit 0edc8cf94) on
    // the assumption that libdaos would safely refcount fini/init pairs;
    // it doesn't.  Any subsequent daos_init() in the same process — e.g.
    // a reader open after the writer close, which the staging-common
    // tests exercise — produced d_hhash_link_lookup errors at init and
    // an obj_ec_update_iod_size assertion during the read.  Refcounting
    // on our side wouldn't help: the failure is in libdaos's reinit
    // path, not in our balance of init/fini calls.  So libdaos stays
    // initialized until process exit, where the OS reaps it.  The
    // per-handle close calls above ARE safe and do the work that
    // matters in long-running multi-engine processes.

    FlushProfiler();
}

void DaosWriter::FlushProfiler()
{
    std::vector<std::string> transportTypes;
    std::vector<std::string> transportNames;
    std::vector<adios2::profiling::IOChrono *> transportProfilers;

    if (m_Comm.Rank() == 0 && m_MetadataFile)
    {
        transportTypes.push_back(m_MetadataFile->m_Type + "_" + m_MetadataFile->m_Library);
        transportNames.push_back(adios2sys::SystemTools::GetFilenameName(m_MetadataFile->m_Name));
        transportProfilers.push_back(&m_MetadataFile->m_Profiler);
    }

    // find first File type output, where we can write the profile
    int fileTransportIdx = -1;
    for (size_t i = 0; i < transportTypes.size(); ++i)
    {
        if (transportTypes[i].compare(0, 4, "File") == 0)
        {
            fileTransportIdx = static_cast<int>(i);
        }
    }

    const std::string lineJSON(
        m_Profiler.GetRankProfilingJSON(transportTypes, transportNames, transportProfilers) +
        ",\n");

    const std::vector<char> profilingJSON(m_Profiler.AggregateProfilingJSON(lineJSON));

    if (m_RankMPI == 0)
    {
        std::string profileFileName;
        std::vector<std::string> bpBaseNames = {m_Name};
        if (fileTransportIdx > -1)
        {
            profileFileName = bpBaseNames[fileTransportIdx] + "/profiling.json";
        }
        else
        {
            profileFileName = bpBaseNames[0] + "_profiling.json";
        }
        transport::FileFStream profilingJSONStream(m_Comm);
        profilingJSONStream.Open(profileFileName, Mode::Write);
        profilingJSONStream.Write(profilingJSON.data(), profilingJSON.size());
        profilingJSONStream.Close();
    }
}

size_t DaosWriter::DebugGetDataBufferSize() const
{
    return m_BP5Serializer.DebugGetDataBufferSize();
}

// True if addr was seen within the last m_WarmthWindow steps; records it.
bool DaosWriter::AddrIsWarm(const void *addr)
{
    AddrWindow &w = m_AddrWindow;
    if (w.slots.size() != m_WarmthWindow)
    {
        w.slots.assign(m_WarmthWindow, {});
        w.cur = -1;
        w.lastStep = -1;
    }
    if (w.lastStep != m_WriterStep)
    {
        w.cur = (w.cur + 1) % static_cast<int>(m_WarmthWindow);
        w.slots[w.cur].clear();
        w.lastStep = m_WriterStep;
    }
    bool warm = false;
    for (const auto &slot : w.slots)
    {
        if (slot.count(addr))
        {
            warm = true;
            break;
        }
    }
    w.slots[w.cur].insert(addr);
    return warm;
}

void DaosWriter::PutCommon(VariableBase &variable, const void *values, bool sync)
{
    profiling::ProfilerGuard pc(m_Profiler, "PutCommon");
    if (!m_BetweenStepPairs)
    {
        BeginStep(StepMode::Update);
    }

    // if the user buffer is allocated on the GPU always use sync mode
    if (variable.GetMemorySpace(values) != MemorySpace::Host)
        sync = true;

    size_t *Shape = NULL;
    size_t *Start = NULL;
    size_t *Count = NULL;
    size_t DimCount = variable.m_Count.size();

    if (variable.m_ShapeID == ShapeID::GlobalArray)
    {
        Shape = variable.m_Shape.data();
        Count = variable.m_Count.data();
        Start = variable.m_Start.data();
    }
    else if (variable.m_ShapeID == ShapeID::LocalArray)
    {
        Count = variable.m_Count.data();
    }
    else if (variable.m_ShapeID == ShapeID::JoinedArray)
    {
        Count = variable.m_Count.data();
        Shape = variable.m_Shape.data();
    }

    size_t ObjSize;
    if (variable.m_Type == DataType::Struct)
    {
        ObjSize = variable.m_ElementSize;
    }
    else
    {
        ObjSize = helper::GetDataTypeSize(variable.m_Type);
    }

    size_t n = helper::GetTotalSize(variable.m_Count) * ObjSize;
    if (!sync)
    {
        /* If arrays is small, force copying to internal buffer to aggregate
         * small writes */
        if (n < m_Parameters.MinDeferredSize)
        {
            sync = true;
        }
    }

    // Warmth guard: skip zero-copy for a never-reused buffer (cold loses).
    if (!sync && m_WarmthWindow > 0 && variable.m_Operations.empty() &&
        variable.m_MemoryCount.empty() && variable.m_Type != DataType::String)
    {
        if (!AddrIsWarm(values))
        {
            sync = true;
        }
    }

    // Tally the zero-copy (External) vs copy outcome.
    if (!sync && variable.m_Operations.empty() && variable.m_MemoryCount.empty() &&
        variable.m_Type != DataType::String)
    {
        ++m_ZeroCopyPuts;
        m_ZeroCopyBytes += n;
    }
    else
    {
        ++m_CopyPuts;
        m_CopyBytes += n;
    }

    if (!variable.m_MemoryCount.empty())
    {
        int DimCount = variable.m_Count.size();
        std::vector<size_t> ZeroDims(DimCount);
        // get a temporary span then fill with memselection now
        format::BufferV::BufferPos bp5span(0, 0, 0);
        {
            profiling::ProfilerGuard m(m_Profiler, "PutCommon_Marshal");
            m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(), variable.m_Type,
                                    variable.m_ElementSize, DimCount, Shape, Count, Start, nullptr,
                                    false, &bp5span);
        }
        void *ptr = m_BP5Serializer.GetPtr(bp5span.bufferIdx, bp5span.posInBuffer);

        const bool sourceRowMajor = helper::IsRowMajor(m_IO.m_HostLanguage);

        helper::NdCopy((const char *)values, helper::CoreDims(ZeroDims), variable.m_MemoryCount,
                       sourceRowMajor, false, (char *)ptr, variable.m_MemoryStart, variable.m_Count,
                       sourceRowMajor, false, ObjSize, helper::CoreDims(), helper::CoreDims(),
                       helper::CoreDims(), helper::CoreDims(), false /* safemode */,
                       variable.m_MemSpace);
    }
    else if (variable.m_Type == DataType::String)
    {
        std::string &source = *(std::string *)values;
        void *p = &(source[0]);
        profiling::ProfilerGuard m(m_Profiler, "PutCommon_Marshal");
        m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(), variable.m_Type,
                                variable.m_ElementSize, DimCount, Shape, Count, Start, &p, sync,
                                nullptr);
    }
    else
    {
        profiling::ProfilerGuard m(m_Profiler, "PutCommon_Marshal");
        m_BP5Serializer.Marshal((void *)&variable, variable.m_Name.c_str(), variable.m_Type,
                                variable.m_ElementSize, DimCount, Shape, Count, Start, values, sync,
                                nullptr);
    }
}

#define declare_type(T)                                                                            \
    void DaosWriter::DoPut(Variable<T> &variable, typename Variable<T>::Span &span,                \
                           const bool initialize, const T &value)                                  \
    {                                                                                              \
        PERFSTUBS_SCOPED_TIMER("DaosWriter::Put");                                                 \
        PutCommonSpan(variable, span, initialize, value);                                          \
    }

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T)                                                                            \
    void DaosWriter::DoPutSync(Variable<T> &variable, const T *data)                               \
    {                                                                                              \
        PutCommon(variable, data, true);                                                           \
    }                                                                                              \
    void DaosWriter::DoPutDeferred(Variable<T> &variable, const T *data)                           \
    {                                                                                              \
        PutCommon(variable, data, false);                                                          \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T, L)                                                                         \
    T *DaosWriter::DoBufferData_##L(const int bufferIdx, const size_t payloadPosition,             \
                                    const size_t bufferID) noexcept                                \
    {                                                                                              \
        return reinterpret_cast<T *>(m_BP5Serializer.GetPtr(bufferIdx, payloadPosition));          \
    }

ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

void DaosWriter::DoPutStructSync(VariableStruct &variable, const void *data)
{
    PutCommon(variable, data, true);
}

void DaosWriter::DoPutStructDeferred(VariableStruct &variable, const void *data)
{
    PutCommon(variable, data, false);
}

void DaosWriter::daos_handle_share(daos_handle_t *hdl, int type)
{
    d_iov_t ghdl = {NULL, 0, 0};
    int rc;

    CALI_MARK_BEGIN("DaosWriter::local2global+broadcast_sizeofhandle");
    if (m_Comm.Rank() == 0)
    {
        /** fetch size of global handle */
        if (type == DaosWriter::HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else
            rc = daos_cont_local2global(*hdl, &ghdl);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "daos_handle_share",
                                              "local2global (size) failed rc=" +
                                                  std::to_string(rc));
        }
    }

    /** broadcast size of global handle to all peers */
    m_Comm.Bcast((uint64_t *)&ghdl.iov_buf_len, 1, 0);
    CALI_MARK_END("DaosWriter::local2global+broadcast_sizeofhandle");

    /** allocate buffer for global pool handle */
    ghdl.iov_buf = malloc(ghdl.iov_buf_len);
    ghdl.iov_len = ghdl.iov_buf_len;

    CALI_MARK_BEGIN("DaosWriter::local2global+broadcast_handle");
    if (m_Comm.Rank() == 0)
    {
        /** generate actual global handle to share with peer tasks */
        if (type == DaosWriter::HANDLE_POOL)
            rc = daos_pool_local2global(*hdl, &ghdl);
        else
            rc = daos_cont_local2global(*hdl, &ghdl);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "daos_handle_share",
                                              "local2global failed rc=" + std::to_string(rc));
        }
    }

    /** broadcast global handle to all peers */
    m_Comm.Bcast((char *)ghdl.iov_buf, ghdl.iov_len, 0);
    CALI_MARK_END("DaosWriter::local2global+broadcast_handle");

    CALI_MARK_BEGIN("DaosWriter::global2local+barrier");
    if (m_Comm.Rank() != 0)
    {
        /** unpack global handle */
        if (type == DaosWriter::HANDLE_POOL)
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
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "daos_handle_share",
                                              "global2local failed rc=" + std::to_string(rc));
        }
    }

    free(ghdl.iov_buf);

    m_Comm.Barrier();
    CALI_MARK_END("DaosWriter::global2local+barrier");
}

void DaosWriter::CreateDaosArrayObject()
{
    int rc;
    CALI_MARK_BEGIN("DaosWriter::create-daos-array");
    /** Open a DAOS array object */
    daos_size_t cell_size = 1;
    daos_size_t chunk_size = 1048576;
    // Server-allocated OIDs avoid PID-derived collisions when multiple
    // runs reuse the same container (DER_EXIST -1004).  Allocate two
    // contiguous OIDs: base+0 for the metadata array, base+1 for the
    // mdsize KV.
    uint64_t base_oid_lo = 0;
    rc = daos_cont_alloc_oids(coh, 2, &base_oid_lo, NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosArrayObject",
                                          "daos_cont_alloc_oids failed rc=" + std::to_string(rc));
    }
    oid.hi = 0;
    oid.lo = base_oid_lo;
    rc = daos_array_generate_oid(coh, &oid, true, 0, 0, 0);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosArrayObject",
                                          "daos_array_generate_oid failed rc=" +
                                              std::to_string(rc));
    }
    rc = daos_array_create(coh, oid, DAOS_TX_NONE, cell_size, chunk_size, &oh, NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosArrayObject",
                                          "daos_array_create failed rc=" + std::to_string(rc));
    }
    CALI_MARK_END("DaosWriter::create-daos-array");

    /** Create a DAOS KV object to store metadata sizes */
    mdsize_oid.hi = 0;
    mdsize_oid.lo = base_oid_lo + 1;
    // cid=0 (OC_UNKNOWN) so DAOS picks an object class consistent with the
    // container's rd_fac.  Aurora's pool requires rd_fac>=3 and rejects
    // OC_SX (single, no replica) with DER_INVAL.
    rc = daos_obj_generate_oid(coh, &mdsize_oid, DAOS_OT_KV_HASHED, 0, 0, 0);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosArrayObject",
                                          "daos_obj_generate_oid (mdsize) failed rc=" +
                                              std::to_string(rc));
    }

    // Open array object
    CALI_MARK_BEGIN("DaosWriter::daos_kv_open");
    rc = daos_kv_open(coh, mdsize_oid, DAOS_OO_RW, &mdsize_oh, NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosArrayObject",
                                          "daos_kv_open (mdsize) failed rc=" + std::to_string(rc));
    }
    CALI_MARK_END("DaosWriter::daos_kv_open");
}

void DaosWriter::CreateDaosKVObject()
{
    /** Open a DAOS KV object */
    int rc;
    // Server-allocated OID — avoids DER_EXIST collisions when the same
    // container is reused across runs.
    uint64_t kv_oid_lo = 0;
    rc = daos_cont_alloc_oids(coh, 1, &kv_oid_lo, NULL);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosKVObject",
                                          "daos_cont_alloc_oids failed rc=" + std::to_string(rc));
    }
    oid.hi = 0;
    oid.lo = kv_oid_lo;
    // cid=0 (OC_UNKNOWN) so DAOS picks an object class consistent with the
    // container's rd_fac (Aurora pool requires rd_fac>=3 and rejects OC_SX).
    rc = daos_obj_generate_oid(coh, &oid, DAOS_OT_KV_HASHED, 0, 0, 0);
    if (rc)
    {
        helper::Throw<std::runtime_error>("Engine", "DaosWriter", "CreateDaosKVObject",
                                          "daos_obj_generate_oid failed rc=" + std::to_string(rc));
    }
}

void DaosWriter::OpenDataArray()
{
    {
        profiling::ProfilerGuard openSetup(m_Profiler, "OpenSetup");
        // Per-rank DAOS array for the data path.  Each rank has
        // its own OID inside the same container that holds metadata; the
        // (rank -> OID) mapping is persisted out-of-band by the engine
        // (currently TODO: wire into the dataset directory's index file).
        // OID is allocated server-side via daos_cont_alloc_oids so multiple
        // ranks/processes don't collide.
        uint64_t oid_lo = 0;
        int rc = daos_cont_alloc_oids(coh, 1, &oid_lo, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "OpenDataArray",
                                              "daos_cont_alloc_oids failed rc=" +
                                                  std::to_string(rc));
        }

        m_DataArrayOid.lo = oid_lo;
        m_DataArrayOid.hi = 0;
        rc = daos_array_generate_oid(coh, &m_DataArrayOid, /*add_attr=*/true,
                                     /*cid=*/0, /*hints=*/0, /*args=*/0);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "OpenDataArray",
                                              "daos_array_generate_oid failed rc=" +
                                                  std::to_string(rc));
        }

        daos_size_t cell_size = 1;
        daos_size_t chunk_size =
            m_Parameters.BufferChunkSize ? m_Parameters.BufferChunkSize : (1ULL << 20);
        rc = daos_array_create(coh, m_DataArrayOid, DAOS_TX_NONE, cell_size, chunk_size,
                               &m_DataArrayOH, NULL);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "OpenDataArray",
                                              "daos_array_create failed rc=" + std::to_string(rc));
        }

        // Create the shared event queue once; DaosChunkV instances
        // constructed each BeginStep borrow it.
        rc = daos_eq_create(&m_DataEQ);
        if (rc)
        {
            helper::Throw<std::runtime_error>("Engine", "DaosWriter", "OpenDataArray",
                                              "daos_eq_create failed rc=" + std::to_string(rc));
        }

        m_DataArrayCursor = 0;
    } // end OpenSetup scope

    // PersistOids is timed separately so we can attribute the
    // OID-gather collective if it ever shows up.
    profiling::ProfilerGuard persistOids(m_Profiler, "PersistOids");
    PersistDataOidsIndex();
}

void DaosWriter::PersistDataOidsIndex()
{
    const int rank = m_Comm.Rank();
    const int size = m_Comm.Size();

    // Each rank contributes (lo, hi); rank 0 collects.
    uint64_t myOid[2] = {m_DataArrayOid.lo, m_DataArrayOid.hi};
    std::vector<uint64_t> allOids;
    if (rank == 0)
    {
        allOids.resize(static_cast<size_t>(size) * 2);
    }
    m_Comm.GatherArrays(myOid, 2, rank == 0 ? allOids.data() : nullptr, 0);

    if (rank != 0)
    {
        return;
    }

    // m_Name is the dataset directory by the time InitTransports has
    // run.  Write a small human-readable text index.  Format is one
    // record per line: "<rank> <oid_lo> <oid_hi>".  Header lines start
    // with '#'.
    std::string indexPath = m_Name + "/data_oids.txt";
    std::ofstream f(indexPath);
    if (!f)
    {
        std::cerr << "DaosWriter: WARNING: failed to write " << indexPath
                  << " (continuing; reader will not find data arrays)" << std::endl;
        return;
    }
    f << "# DAOS per-rank data array OID index\n";
    f << "# format: rank oid_lo oid_hi\n";
    f << "# nranks: " << size << "\n";
    f << "# pool: " << m_pool_label << "\n";
    f << "# container: " << m_cont_label << "\n";
    for (int r = 0; r < size; ++r)
    {
        f << r << " " << allOids[2 * r] << " " << allOids[2 * r + 1] << "\n";
    }
    f.close();
}

void DaosWriter::CloseDataArray()
{
    if (m_DataArrayOH.cookie != 0)
    {
        daos_array_close(m_DataArrayOH, NULL);
        m_DataArrayOH = {};
    }
    if (m_DataEQ.cookie != 0)
    {
        daos_eq_destroy(m_DataEQ, /*flags=*/0);
        m_DataEQ = {};
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
