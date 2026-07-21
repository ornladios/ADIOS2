/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_ENGINE_DAOS_DAOSWRITER_H_
#define ADIOS2_ENGINE_DAOS_DAOSWRITER_H_
#define DSS_PSETID "daos_server"

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/CoreTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/engine/daos/DaosEngine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosMemory.h" // PaddingToAlignOffset
#include "adios2/toolkit/format/bp5/BP5Serializer.h"
#include "adios2/toolkit/format/buffer/BufferV.h"
#include "adios2/toolkit/format/buffer/chunk/DaosChunkV.h"
#include "adios2/toolkit/transportman/TransportMan.h"
#if defined(ADIOS2_HAVE_Caliper)
#include <caliper/cali-manager.h>
#include <caliper/cali.h>
#else
#define CALI_MARK_BEGIN(x)
#define CALI_MARK_END(x)
#endif
#include <daos.h>
#include <daos_obj.h>

#include <cstdint>
#include <unordered_set>
#include <vector>

#define MAX_AGGREGATE_METADATA_SIZE (5'368'709'120ULL)
#define chunk_size_1mb 1'048'576

namespace adios2
{
namespace core
{
namespace engine
{

class DaosWriter : public DaosEngine, public core::Engine
{

public:
    /**
     * Constructor for file Writer in Daos format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param comm multi-process communicator
     */
    DaosWriter(IO &io, const std::string &name, const Mode mode, helper::Comm comm);

    ~DaosWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void PerformDataWrite() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

    size_t DebugGetDataBufferSize() const final;

private:
    /** Single object controlling BP buffering */
    format::BP5Serializer m_BP5Serializer;

    std::shared_ptr<Transport> m_MetadataIndexFile;
    std::shared_ptr<Transport> m_MetaMetadataFile;
    std::shared_ptr<Transport> m_MetadataFile;

    /* DAOS declarations */

    char m_pool_label[100], m_cont_label[100];

    /* Declare variables for pool and container handles */
    daos_handle_t poh{}, coh{};

    enum DAOS_handleType
    {
        HANDLE_POOL,
        HANDLE_CO,
    };

    /* Declare variables for the KV object */
    daos_handle_t oh{}, mdsize_oh{};
    daos_obj_id_t oid, mdsize_oid;
    daos_array_iod_t iod;
    daos_range_t rg;
    d_sg_list_t sgl;
    d_iov_t iov;

    /* Per-rank DAOS data array.  BeginStep constructs a DaosChunkV
     * that submits async daos_array_writes during AddToVec (overlapping
     * with subsequent Puts), and WriteData drains and writes the small
     * remainder synchronously with one daos_array_write at EndStep. */
    daos_obj_id_t m_DataArrayOid{};
    daos_handle_t m_DataArrayOH{};
    /// Engine-lifetime DAOS event queue shared across DaosChunkV instances.
    /// Avoids paying daos_eq_create (~300 ms) per BeginStep.
    daos_handle_t m_DataEQ{};
    /// Per-rank cursor into the DAOS data array; persists across steps.
    daos_size_t m_DataArrayCursor = 0;
    /// Engine-lifetime data buffer.  Allocated lazily on the first
    /// BeginStep that needs it; reused (Reset()) on every subsequent step
    /// so its chunks keep stable addresses → libfabric MR cache stays hot,
    /// no per-step page-fault zeroing on chunk first-touch.
    std::unique_ptr<format::DaosChunkV> m_DataBuf;

    void OpenDataArray();
    void CloseDataArray();
    /// Gather all ranks' (rank, oid_lo, oid_hi) and write a tiny text index
    /// at <m_Name>/data_oids.txt so the reader can locate each rank's array.
    void PersistDataOidsIndex();

    /// All per-phase timers live in m_Profiler (BP5-style JSONProfiler).
    /// RegisterProfilerTimers is called once at engine open to AddTimerWatch
    /// every name used at the call sites.  Names are documented inline at
    /// each ProfilerGuard, and at the JSON field key in profiling.json.
    void RegisterProfilerTimers();
    /// Copy accumulators that live outside m_Profiler (BP5Serializer's
    /// sub-Marshal counters and DaosChunkV's fresh-allocation count) into
    /// the corresponding registered timers so they are emitted to
    /// profiling.json by the standard FlushProfiler path.  Called once at
    /// DoClose, before FlushProfiler.
    void SyncExternalCountersToProfiler();

    /// How per-rank metadata is laid out in DAOS.  Selectable via the
    /// DAOS_METADATA_LAYOUT environment variable; ARRAY_1MB_ALIGNED is
    /// the default.  The data path is identical across all values; only
    /// metadata storage differs.
    enum class MetadataLayout
    {
        ARRAY,
        ARRAY_1MB_ALIGNED,
        KV
    };

    MetadataLayout metadataLayout;
    void SetMetadataLayout();
    void SetPoolAndContName();

    /// Phase-2 gating: when true, ES_Gate Allreduce and ES_SelAgg
    /// BP5AggregateInformation are skipped, and each rank serializes
    /// its own NewMetaMetaBlocks + AttributeEncodeBuffer into its
    /// per-rank metadata blob.  Selected by DAOS_PER_RANK_METADATA env
    /// var (default 0 = aggregated, today's Phase-1 behavior).
    bool m_PerRankMetadata = false;
    void SetPerRankMetadata();

    size_t m_step_offset = 0;

    // Declare WriteMetadata function
    void WriteMetadata(format::BP5Serializer::TimestepInfo &);
    void DaosArrayWriteMetadata(format::BP5Serializer::TimestepInfo &);
    void DaosKVWriteMetadata(format::BP5Serializer::TimestepInfo &);
    /// Compose this rank's per-step metadata blob.  In aggregated mode
    /// (Phase 1 default): [MetaEncodeBuffer][step_start u64], 8-byte
    /// trailer.  In per-rank mode (Phase 2): [MetaEncode][MetaMetaBlocks
    /// serialized][AttributeEncodeBuffer][footer], where footer is
    /// (mmb_size, attr_size, step_start) = 24 bytes.  Returns a single
    /// contiguous buffer; size determined by the caller from the result.
    std::vector<char> BuildMetadataBlob(format::BP5Serializer::TimestepInfo &TSInfo);
    void CreateDaosArrayObject();
    void CreateDaosKVObject();
    void OpenDaosObjAndShare();
    void WriteObjectIDsToFile();

    char node[128] = "unknown";

    int64_t m_WriterStep = 0;

    std::vector<std::string> m_MetadataFileNames;
    std::vector<std::string> m_MetaMetadataFileNames;
    std::vector<std::string> m_MetadataIndexFileNames;
    std::string m_OIDFileName;

    bool m_BetweenStepPairs = false;

    void Init() final;

    /** Parses parameters from IO SetParameters */
    void InitParameters() final;
    /** Complete opening/createing metadata files */
    void InitTransports() final;
    /** DAOS pool connection and container opening */
    void InitDAOS();
    void array_oh_share(daos_handle_t *);
    /** Allocates memory and starts a PG group */
    void InitBPBuffer();
    void NotifyEngineAttribute(std::string name, DataType type) noexcept;
    /** Notify the engine when a new attribute is defined or modified. Called
     * from IO.tcc
     */
    void NotifyEngineAttribute(std::string name, AttributeBase *Attr, void *data) noexcept;

#define declare_type(T)                                                                            \
    void DoPut(Variable<T> &variable, typename Variable<T>::Span &span, const bool initialize,     \
               const T &value) final;

    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutCommonSpan(Variable<T> &variable, typename Variable<T>::Span &span,
                       const bool initialize, const T &value);

#define declare_type(T)                                                                            \
    void DoPutSync(Variable<T> &, const T *) final;                                                \
    void DoPutDeferred(Variable<T> &, const T *) final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void PutCommon(VariableBase &variable, const void *data, bool sync);

#define declare_type(T, L)                                                                         \
    T *DoBufferData_##L(const int bufferIdx, const size_t payloadPosition,                         \
                        const size_t bufferID = 0) noexcept final;

    ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

    void DoPutStructSync(VariableStruct &, const void *) final;
    void DoPutStructDeferred(VariableStruct &, const void *) final;

    void PutStruct(VariableStruct &, const void *, bool);

    void FlushData(const bool isFinal = false);

    void DoClose(const int transportIndex = -1) final;

    /** Write a profiling.json file from m_BP1Writer and m_TransportsManager
     * profilers*/
    void WriteProfilingJSONFile();

    void WriteMetaMetadata(const std::vector<format::BP5Base::MetaMetaInfoBlock> MetaMetaBlocks);

    void WriteMetadataFileIndex(uint64_t MetaDataPos, uint64_t MetaDataSize);

    uint64_t WriteAttributes(const std::vector<core::iovec> &AttributeBlocks);

    /** Drain the in-flight DaosChunkV writes and flush the tail. */
    void WriteData(format::BufferV *Data);

    void UpdateActiveFlag(const bool active);

    void MarshalAttributes();

    void FlushProfiler();

    adios2::profiling::JSONProfiler m_Profiler;

protected:
    virtual void DestructorClose(bool Verbose) noexcept;

private:
    // updated during WriteMetaData
    uint64_t m_MetaDataPos = 0;

    /** Per-rank cursor into the data array at the start of this step.
     *  Snapshotted in BeginStep so that, regardless of intermediate
     *  PerformDataWrite calls, each rank knows where its step's data
     *  begins.  Written as an 8-byte trailer on the per-rank metadata
     *  blob; the reader uses it as the base offset for ReadData.
     */
    uint64_t m_StepStartDataPos = 0;

    /*
     *  Total data written this timestep
     */
    uint64_t m_ThisTimestepDataSize = 0;

    // Zero-copy vs copy Put split, emitted as DAOS_* timers in profiling.json.
    uint64_t m_ZeroCopyPuts = 0;
    uint64_t m_ZeroCopyBytes = 0;
    uint64_t m_CopyPuts = 0;
    uint64_t m_CopyBytes = 0;

    // Warmth guard: a large buffer zero-copied cold (never-reused, so its MR
    // is unregistered) loses to a copy, so only zero-copy buffers that have
    // recurred. Tracked global-by-address to match the VA-keyed MR cache;
    // window depth covers multi-buffering. Env DAOS_WARMTH_WINDOW; 0 = off.
    size_t m_WarmthWindow = 8;
    struct AddrWindow
    {
        int64_t lastStep = -1;
        int cur = -1;
        std::vector<std::unordered_set<const void *>> slots;
    };
    AddrWindow m_AddrWindow;
    bool AddrIsWarm(const void *addr);

    bool m_MarshalAttributesNecessary = true;

    void MakeHeader(std::vector<char> &buffer, size_t &position, const std::string fileType,
                    const bool isActive);

    // Latest metadata position/size for the metadata index file.
    uint64_t m_LatestMetaDataPos = 0;
    uint64_t m_LatestMetaDataSize = 0;
    /// Tracks whether WriteMetadataFileIndex has been called yet.
    /// In aggregated mode MetaDataPos==0 doubles as the first-call
    /// sentinel; in per-rank mode every call has MetaDataPos==0 so
    /// we need an explicit flag.
    bool m_FirstIndexCall = true;
    TimePoint m_EngineStart;
    TimePoint m_BeginStepStart;

    void daos_handle_share(daos_handle_t *, int);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DAOS_DAOSWRITER_H_ */
