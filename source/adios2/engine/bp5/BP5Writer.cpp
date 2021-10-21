/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.cpp
 *
 */

#include "BP5Writer.h"
#include "BP5Writer.tcc"

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/IO.h"
#include "adios2/helper/adiosFunctions.h" //CheckIndexRange
#include "adios2/toolkit/format/buffer/chunk/ChunkV.h"
#include "adios2/toolkit/format/buffer/malloc/MallocV.h"
#include "adios2/toolkit/transport/file/FileFStream.h"
#include <adios2-perfstubs-interface.h>

#include <ctime>
#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

using namespace adios2::format;

BP5Writer::BP5Writer(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("BP5Writer", io, name, mode, std::move(comm)), m_BP5Serializer(),
  m_FileDataManager(m_Comm), m_FileMetadataManager(m_Comm),
  m_FileMetadataIndexManager(m_Comm), m_FileMetaMetadataManager(m_Comm),
  m_Profiler(m_Comm)
{
    PERFSTUBS_SCOPED_TIMER("BP5Writer::Open");
    m_IO.m_ReadStreaming = false;
    m_EndMessage = " in call to IO Open BP5Writer " + m_Name + "\n";

    Init();
}

StepStatus BP5Writer::BeginStep(StepMode mode, const float timeoutSeconds)
{
    if (m_BetweenStepPairs)
    {
        throw std::logic_error("ERROR: BeginStep() is called a second time "
                               "without an intervening EndStep()");
    }

    m_BetweenStepPairs = true;
    if (m_Parameters.BufferVType == (int)BufferVType::MallocVType)
    {
        m_BP5Serializer.InitStep(new MallocV("BP5Writer", false,
                                             m_Parameters.InitialBufferSize,
                                             m_Parameters.GrowthFactor));
    }
    else
    {
        m_BP5Serializer.InitStep(new ChunkV("BP5Writer",
                                            false /* always copy */,
                                            m_Parameters.BufferChunkSize));
    }
    m_ThisTimestepDataSize = 0;

    return StepStatus::OK;
}

size_t BP5Writer::CurrentStep() const { return m_WriterStep; }

void BP5Writer::PerformPuts()
{
    PERFSTUBS_SCOPED_TIMER("BP5Writer::PerformPuts");
    m_Profiler.Start("PP");
    m_BP5Serializer.PerformPuts();
    m_Profiler.Stop("PP");
    return;
}

void BP5Writer::WriteMetaMetadata(
    const std::vector<format::BP5Base::MetaMetaInfoBlock> MetaMetaBlocks)
{
    for (auto &b : MetaMetaBlocks)
    {
        m_FileMetaMetadataManager.WriteFiles((char *)&b.MetaMetaIDLen,
                                             sizeof(size_t));
        m_FileMetaMetadataManager.WriteFiles((char *)&b.MetaMetaInfoLen,
                                             sizeof(size_t));
        m_FileMetaMetadataManager.WriteFiles((char *)b.MetaMetaID,
                                             b.MetaMetaIDLen);
        m_FileMetaMetadataManager.WriteFiles((char *)b.MetaMetaInfo,
                                             b.MetaMetaInfoLen);
    }
}

uint64_t
BP5Writer::WriteMetadata(const std::vector<core::iovec> &MetaDataBlocks,
                         const std::vector<core::iovec> &AttributeBlocks)
{
    uint64_t MDataTotalSize = 0;
    uint64_t MetaDataSize = 0;
    std::vector<uint64_t> SizeVector;
    std::vector<uint64_t> AttrSizeVector;
    SizeVector.reserve(MetaDataBlocks.size());
    for (auto &b : MetaDataBlocks)
    {
        MDataTotalSize += sizeof(uint64_t) + b.iov_len;
        SizeVector.push_back(b.iov_len);
    }
    for (auto &b : AttributeBlocks)
    {
        MDataTotalSize += sizeof(uint64_t) + b.iov_len;
        AttrSizeVector.push_back(b.iov_len);
    }
    MetaDataSize = 0;
    m_FileMetadataManager.WriteFiles((char *)&MDataTotalSize, sizeof(uint64_t));
    MetaDataSize += sizeof(uint64_t);
    m_FileMetadataManager.WriteFiles((char *)SizeVector.data(),
                                     sizeof(uint64_t) * SizeVector.size());
    MetaDataSize += sizeof(uint64_t) * AttrSizeVector.size();
    m_FileMetadataManager.WriteFiles((char *)AttrSizeVector.data(),
                                     sizeof(uint64_t) * AttrSizeVector.size());
    MetaDataSize += sizeof(uint64_t) * AttrSizeVector.size();
    for (auto &b : MetaDataBlocks)
    {
        if (!b.iov_base)
            continue;
        m_FileMetadataManager.WriteFiles((char *)b.iov_base, b.iov_len);
        MetaDataSize += b.iov_len;
    }

    for (auto &b : AttributeBlocks)
    {
        if (!b.iov_base)
            continue;
        m_FileMetadataManager.WriteFiles((char *)b.iov_base, b.iov_len);
        MetaDataSize += b.iov_len;
    }

    m_MetaDataPos += MetaDataSize;
    return MetaDataSize;
}

void BP5Writer::WriteData(format::BufferV *Data)
{
    switch (m_Parameters.AggregationType)
    {
    case (int)AggregationType::EveryoneWrites:
        WriteData_EveryoneWrites(Data, false);
        break;
    case (int)AggregationType::EveryoneWritesSerial:
        WriteData_EveryoneWrites(Data, true);
        break;
    case (int)AggregationType::TwoLevelShm:
        WriteData_TwoLevelShm(Data);
        break;
    default:
        throw std::invalid_argument(
            "Aggregation method " +
            std::to_string(m_Parameters.AggregationType) +
            "is not supported in BP5");
    }
}

void BP5Writer::WriteData_EveryoneWrites(format::BufferV *Data,
                                         bool SerializedWriters)
{
    const aggregator::MPIChain *a =
        dynamic_cast<aggregator::MPIChain *>(m_Aggregator);

    std::vector<core::iovec> DataVec = Data->DataVec();

    // new step writing starts at offset m_DataPos on aggregator
    // others will wait for the position to arrive from the rank below

    if (a->m_Comm.Rank() > 0)
    {
        a->m_Comm.Recv(&m_DataPos, 1, a->m_Comm.Rank() - 1, 0,
                       "Chain token in BP5Writer::WriteData");
    }

    // align to PAGE_SIZE
    m_DataPos += helper::PaddingToAlignOffset(m_DataPos,
                                              m_Parameters.FileSystemPageSize);
    m_StartDataPos = m_DataPos;

    if (!SerializedWriters && a->m_Comm.Rank() < a->m_Comm.Size() - 1)
    {
        /* Send the token before writing so everyone can start writing asap */
        uint64_t nextWriterPos = m_DataPos + Data->Size();
        a->m_Comm.Isend(&nextWriterPos, 1, a->m_Comm.Rank() + 1, 0,
                        "Chain token in BP5Writer::WriteData");
    }

    m_FileDataManager.WriteFileAt(DataVec.data(), DataVec.size(),
                                  m_StartDataPos);
    m_DataPos += Data->Size();

    if (SerializedWriters && a->m_Comm.Rank() < a->m_Comm.Size() - 1)
    {
        /* send token now, effectively serializing the writers in the chain */
        uint64_t nextWriterPos = m_DataPos;
        a->m_Comm.Isend(&nextWriterPos, 1, a->m_Comm.Rank() + 1, 0,
                        "Chain token in BP5Writer::WriteData");
    }

    if (a->m_Comm.Size() > 1)
    {
        // at the end, last rank sends back the final data pos to first rank
        // so it can update its data pos
        if (a->m_Comm.Rank() == a->m_Comm.Size() - 1)
        {
            a->m_Comm.Isend(&m_DataPos, 1, 0, 0,
                            "Final chain token in BP5Writer::WriteData");
        }
        if (a->m_Comm.Rank() == 0)
        {
            a->m_Comm.Recv(&m_DataPos, 1, a->m_Comm.Size() - 1, 0,
                           "Chain token in BP5Writer::WriteData");
        }
    }
}

void BP5Writer::WriteMetadataFileIndex(uint64_t MetaDataPos,
                                       uint64_t MetaDataSize)
{

    m_FileMetadataManager.FlushFiles();

    std::vector<uint64_t> buf;
    buf.resize(3 + ((FlushPosSizeInfo.size() * 2) + 1) * m_Comm.Size());
    buf[0] = MetaDataPos;
    buf[1] = MetaDataSize;
    buf[2] = FlushPosSizeInfo.size();

    uint64_t pos = 3;

    for (int writer = 0; writer < m_Comm.Size(); writer++)
    {
        for (size_t flushNum = 0; flushNum < FlushPosSizeInfo.size();
             flushNum++)
        {
            buf[pos + (flushNum * 2)] = FlushPosSizeInfo[flushNum][2 * writer];
            buf[pos + (flushNum * 2) + 1] =
                FlushPosSizeInfo[flushNum][2 * writer + 1];
        }
        buf[pos + FlushPosSizeInfo.size() * 2] = m_WriterDataPos[writer];
        pos += (FlushPosSizeInfo.size() * 2) + 1;
    }

    m_FileMetadataIndexManager.WriteFiles((char *)buf.data(),
                                          buf.size() * sizeof(uint64_t));

#ifdef DUMPDATALOCINFO
    std::cout << "Flush count is :" << FlushPosSizeInfo.size() << std::endl;
    std::cout << "Write Index positions = {" << std::endl;

    for (size_t i = 0; i < m_Comm.Size(); ++i)
    {
        std::cout << "Writer " << i << " has data at: " << std::endl;
        uint64_t eachWriterSize = FlushPosSizeInfo.size() * 2 + 1;
        for (size_t j = 0; j < FlushPosSizeInfo.size(); ++j)
        {
            std::cout << "loc:" << buf[3 + eachWriterSize * i + j * 2]
                      << " siz:" << buf[3 + eachWriterSize * i + j * 2 + 1]
                      << std::endl;
        }
        std::cout << "loc:" << buf[3 + eachWriterSize * (i + 1) - 1]
                  << std::endl;
    }
    std::cout << "}" << std::endl;
#endif
    /* reset for next timestep */
    FlushPosSizeInfo.clear();
}

void BP5Writer::MarshalAttributes()
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    const auto &attributes = m_IO.GetAttributes();

    const uint32_t attributesCount = static_cast<uint32_t>(attributes.size());

    // if there are no new attributes, nothing to do
    if (attributesCount == m_MarshaledAttributesCount)
    {
        return;
    }

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
            core::Attribute<std::string> &attribute =
                *m_IO.InquireAttribute<std::string>(name);
            void *data_addr;
            if (attribute.m_IsSingleValue)
            {
                data_addr = (void *)attribute.m_DataSingleValue.c_str();
            }
            else
            {
                const char **tmp =
                    (const char **)malloc(sizeof(char *) * element_count);
                for (int i = 0; i < element_count; i++)
                {
                    auto str = &attribute.m_DataArray[i];
                    tmp[i] = str->c_str();
                }
                // tmp will be free'd after final attribute marshalling
                data_addr = (void *)tmp;
            }

            m_BP5Serializer.MarshalAttribute(name.c_str(), type, sizeof(char *),
                                             element_count, data_addr);
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        core::Attribute<T> &attribute = *m_IO.InquireAttribute<T>(name);       \
        int element_count = -1;                                                \
        void *data_addr = &attribute.m_DataSingleValue;                        \
        if (!attribute.m_IsSingleValue)                                        \
        {                                                                      \
            element_count = attribute.m_Elements;                              \
            data_addr = attribute.m_DataArray.data();                          \
        }                                                                      \
        m_BP5Serializer.MarshalAttribute(attribute.m_Name.c_str(), type,       \
                                         sizeof(T), element_count, data_addr); \
    }

        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
    m_MarshaledAttributesCount = attributesCount;
}

void BP5Writer::EndStep()
{
    m_BetweenStepPairs = false;
    PERFSTUBS_SCOPED_TIMER("BP5Writer::EndStep");
    m_Profiler.Start("endstep");
    MarshalAttributes();

    // true: advances step
    auto TSInfo = m_BP5Serializer.CloseTimestep(m_WriterStep);

    /* TSInfo includes NewMetaMetaBlocks, the MetaEncodeBuffer, the
     * AttributeEncodeBuffer and the data encode Vector */
    /* the first */

    m_Profiler.Start("AWD");
    WriteData(TSInfo.DataBuffer);
    m_Profiler.Stop("AWD");

    m_ThisTimestepDataSize += TSInfo.DataBuffer->Size();

    std::vector<char> MetaBuffer = m_BP5Serializer.CopyMetadataToContiguous(
        TSInfo.NewMetaMetaBlocks, TSInfo.MetaEncodeBuffer,
        TSInfo.AttributeEncodeBuffer, m_ThisTimestepDataSize, m_StartDataPos);

    size_t LocalSize = MetaBuffer.size();
    std::vector<size_t> RecvCounts = m_Comm.GatherValues(LocalSize, 0);

    std::vector<char> *RecvBuffer = new std::vector<char>;
    if (m_Comm.Rank() == 0)
    {
        uint64_t TotalSize = 0;
        for (auto &n : RecvCounts)
            TotalSize += n;
        RecvBuffer->resize(TotalSize);
    }

    m_Profiler.Start("meta_gather");
    m_Comm.GathervArrays(MetaBuffer.data(), LocalSize, RecvCounts.data(),
                         RecvCounts.size(), RecvBuffer->data(), 0);
    m_Profiler.Stop("meta_gather");

    if (m_Comm.Rank() == 0)
    {
        std::vector<format::BP5Base::MetaMetaInfoBlock> UniqueMetaMetaBlocks;
        std::vector<uint64_t> DataSizes;
        std::vector<core::iovec> AttributeBlocks;
        auto Metadata = m_BP5Serializer.BreakoutContiguousMetadata(
            RecvBuffer, RecvCounts, UniqueMetaMetaBlocks, AttributeBlocks,
            DataSizes, m_WriterDataPos);
        if (m_MetaDataPos == 0)
        {
            //  First time, write the headers
            format::BufferSTL b;
            MakeHeader(b, "Metadata", false);
            m_FileMetadataManager.WriteFiles(b.m_Buffer.data(), b.m_Position);
            m_MetaDataPos = b.m_Position;
            format::BufferSTL bi;
            MakeHeader(bi, "Index Table", true);
            m_FileMetadataIndexManager.WriteFiles(bi.m_Buffer.data(),
                                                  bi.m_Position);
            // where each rank's data will end up
            m_FileMetadataIndexManager.WriteFiles((char *)m_Assignment.data(),
                                                  sizeof(m_Assignment[0]) *
                                                      m_Assignment.size());
        }
        WriteMetaMetadata(UniqueMetaMetaBlocks);
        uint64_t ThisMetaDataPos = m_MetaDataPos;
        uint64_t ThisMetaDataSize = WriteMetadata(Metadata, AttributeBlocks);
        WriteMetadataFileIndex(ThisMetaDataPos, ThisMetaDataSize);
    }
    delete RecvBuffer;
    m_Profiler.Stop("endstep");
    m_WriterStep++;
}

// PRIVATE
void BP5Writer::Init()
{
    m_BP5Serializer.m_Engine = this;
    m_RankMPI = m_Comm.Rank();
    InitParameters();
    InitAggregator();
    InitTransports();
    InitBPBuffer();
}

#define declare_type(T)                                                        \
    void BP5Writer::DoPut(Variable<T> &variable,                               \
                          typename Variable<T>::Span &span,                    \
                          const bool initialize, const T &value)               \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER("BP5Writer::Put");                              \
        PutCommonSpan(variable, span, initialize, value);                      \
    }

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T)                                                        \
    void BP5Writer::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutCommon(variable, data, true);                                       \
    }                                                                          \
    void BP5Writer::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutCommon(variable, data, false);                                      \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T, L)                                                     \
    T *BP5Writer::DoBufferData_##L(const int bufferIdx,                        \
                                   const size_t payloadPosition,               \
                                   const size_t bufferID) noexcept             \
    {                                                                          \
        return reinterpret_cast<T *>(                                          \
            m_BP5Serializer.GetPtr(bufferIdx, payloadPosition));               \
    }

ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

void BP5Writer::InitParameters()
{
    ParseParams(m_IO, m_Parameters);
    m_WriteToBB = !(m_Parameters.BurstBufferPath.empty());
    m_DrainBB = m_WriteToBB && m_Parameters.BurstBufferDrain;

    if (m_Parameters.NumAggregators > static_cast<unsigned int>(m_Comm.Size()))
    {
        m_Parameters.NumAggregators = static_cast<unsigned int>(m_Comm.Size());
    }

    if (m_Parameters.NumSubFiles > m_Parameters.NumAggregators)
    {
        m_Parameters.NumSubFiles = m_Parameters.NumAggregators;
    }

    if (m_Parameters.FileSystemPageSize == 0)
    {
        m_Parameters.FileSystemPageSize = 4096;
    }
    if (m_Parameters.FileSystemPageSize > 67108864)
    {
        // Limiting to max 64MB page size
        m_Parameters.FileSystemPageSize = 67108864;
    }
}

void BP5Writer::InitAggregator()
{
    // in BP5, aggregation is "always on", but processes may be alone, so
    // m_Aggregator.m_IsActive is always true
    // m_Aggregator.m_Comm.Rank() will always succeed (not abort)
    // m_Aggregator.m_SubFileIndex is always set
    if (m_Parameters.AggregationType == (int)AggregationType::EveryoneWrites ||
        m_Parameters.AggregationType ==
            (int)AggregationType::EveryoneWritesSerial)
    {
        m_Parameters.NumSubFiles = m_Parameters.NumAggregators;
        m_AggregatorEveroneWrites.Init(m_Parameters.NumAggregators,
                                       m_Parameters.NumSubFiles, m_Comm);
        m_IAmDraining = m_AggregatorEveroneWrites.m_IsAggregator;
        m_IAmWritingDataHeader = m_AggregatorEveroneWrites.m_IsAggregator;
        m_IAmWritingData = true;
        DataWritingComm = &m_AggregatorEveroneWrites.m_Comm;
        m_Aggregator = static_cast<aggregator::MPIAggregator *>(
            &m_AggregatorEveroneWrites);
    }
    else
    {
        size_t numNodes = m_AggregatorTwoLevelShm.PreInit(m_Comm);
        (void)numNodes;
        m_AggregatorTwoLevelShm.Init(m_Parameters.NumAggregators,
                                     m_Parameters.NumSubFiles, m_Comm);

        /*std::cout << "Rank " << m_RankMPI << " aggr? "
                  << m_AggregatorTwoLevelShm.m_IsAggregator << " master? "
                  << m_AggregatorTwoLevelShm.m_IsMasterAggregator
                  << " aggr size = " << m_AggregatorTwoLevelShm.m_Size
                  << " rank = " << m_AggregatorTwoLevelShm.m_Rank
                  << " subfile = " << m_AggregatorTwoLevelShm.m_SubStreamIndex
                  << " type = " << m_Parameters.AggregationType

                  << std::endl;*/

        m_IAmDraining = m_AggregatorTwoLevelShm.m_IsMasterAggregator;
        m_IAmWritingData = m_AggregatorTwoLevelShm.m_IsAggregator;
        m_IAmWritingDataHeader = m_AggregatorTwoLevelShm.m_IsMasterAggregator;
        DataWritingComm = &m_AggregatorTwoLevelShm.m_AggregatorChainComm;
        m_Aggregator =
            static_cast<aggregator::MPIAggregator *>(&m_AggregatorTwoLevelShm);
    }
}

void BP5Writer::InitTransports()
{
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    if (m_WriteToBB)
    {
        m_BBName = m_Parameters.BurstBufferPath + PathSeparator + m_Name;
    }
    else
    {
        m_BBName = m_Name;
    }
    /* From this point, engine writes to m_BBName, which points to either
        the BB file system if BB is turned on, or to the target file system.
        m_Name always points to the target file system, to which the drainer
        should write if BB is turned on
    */

    // Names passed to IO AddTransport option with key "Name"
    const std::vector<std::string> transportsNames =
        m_FileDataManager.GetFilesBaseNames(m_BBName,
                                            m_IO.m_TransportsParameters);

    // /path/name.bp.dir/name.bp.rank
    m_SubStreamNames =
        GetBPSubStreamNames(transportsNames, m_Aggregator->m_SubStreamIndex);

    if (m_IAmDraining)
    {
        // Only (master)aggregators will run draining processes
        if (m_DrainBB)
        {
            const std::vector<std::string> drainTransportNames =
                m_FileDataManager.GetFilesBaseNames(
                    m_Name, m_IO.m_TransportsParameters);
            m_DrainSubStreamNames = GetBPSubStreamNames(
                drainTransportNames, m_Aggregator->m_SubStreamIndex);
            /* start up BB thread */
            //            m_FileDrainer.SetVerbose(
            //				     m_Parameters.BurstBufferVerbose,
            //				     m_Comm.Rank());
            m_FileDrainer.Start();
        }
    }

    /* Create the directories either on target or burst buffer if used */
    //    m_BP4Serializer.m_Profiler.Start("mkdir");

    if (m_Comm.Rank() == 0)
    {
        m_MetadataFileNames = GetBPMetadataFileNames(transportsNames);
        m_MetaMetadataFileNames = GetBPMetaMetadataFileNames(transportsNames);
        m_MetadataIndexFileNames = GetBPMetadataIndexFileNames(transportsNames);
    }
    m_FileMetadataManager.MkDirsBarrier(m_MetadataFileNames,
                                        m_IO.m_TransportsParameters,
                                        m_Parameters.NodeLocal || m_WriteToBB);
    if (m_DrainBB)
    {
        /* Create the directories on target anyway by main thread */
        m_FileDataManager.MkDirsBarrier(m_DrainSubStreamNames,
                                        m_IO.m_TransportsParameters,
                                        m_Parameters.NodeLocal);
    }

    /* Everyone opens its data file. Each aggregation chain opens
       one data file and does so in chain, not everyone at once */
    if (m_Parameters.AsyncTasks)
    {
        for (size_t i = 0; i < m_IO.m_TransportsParameters.size(); ++i)
        {
            m_IO.m_TransportsParameters[i]["asynctasks"] = "true";
        }
    }

    bool useProfiler = true;

    if (m_IAmWritingData)
    {
        m_FileDataManager.OpenFiles(m_SubStreamNames, m_OpenMode,
                                    m_IO.m_TransportsParameters, useProfiler,
                                    *DataWritingComm);
    }

    if (m_IAmDraining)
    {
        if (m_DrainBB)
        {
            for (const auto &name : m_DrainSubStreamNames)
            {
                m_FileDrainer.AddOperationOpen(name, m_OpenMode);
            }
        }
    }

    if (m_Comm.Rank() == 0)
    {
        m_FileMetaMetadataManager.OpenFiles(m_MetaMetadataFileNames, m_OpenMode,
                                            m_IO.m_TransportsParameters,
                                            useProfiler);

        m_FileMetadataManager.OpenFiles(m_MetadataFileNames, m_OpenMode,
                                        m_IO.m_TransportsParameters,
                                        useProfiler);

        m_FileMetadataIndexManager.OpenFiles(
            m_MetadataIndexFileNames, m_OpenMode, m_IO.m_TransportsParameters,
            useProfiler);

        if (m_DrainBB)
        {
            const std::vector<std::string> drainTransportNames =
                m_FileDataManager.GetFilesBaseNames(
                    m_Name, m_IO.m_TransportsParameters);
            m_DrainMetadataFileNames =
                GetBPMetadataFileNames(drainTransportNames);
            m_DrainMetadataIndexFileNames =
                GetBPMetadataIndexFileNames(drainTransportNames);

            for (const auto &name : m_DrainMetadataFileNames)
            {
                m_FileDrainer.AddOperationOpen(name, m_OpenMode);
            }
            for (const auto &name : m_DrainMetadataIndexFileNames)
            {
                m_FileDrainer.AddOperationOpen(name, m_OpenMode);
            }
        }
    }

    // last process create .bpversion file with content "5"
    if (m_Comm.Rank() == m_Comm.Size() - 1)
    {
        std::vector<std::string> versionNames =
            GetBPVersionFileNames(transportsNames);
        auto emptyComm = helper::Comm();
        transportman::TransportMan tm(emptyComm);
        tm.OpenFiles(versionNames, Mode::Write, m_IO.m_TransportsParameters,
                     useProfiler);
        char b[1] = {'5'};
        tm.WriteFiles(b, 1);
    }
}

/*generate the header for the metadata index file*/
void BP5Writer::MakeHeader(format::BufferSTL &b, const std::string fileType,
                           const bool isActive)
{
    auto lf_CopyVersionChar = [](const std::string version,
                                 std::vector<char> &buffer, size_t &position) {
        helper::CopyToBuffer(buffer, position, version.c_str());
    };

    auto &buffer = b.m_Buffer;
    auto &position = b.m_Position;
    auto &absolutePosition = b.m_AbsolutePosition;
    if (position > 0)
    {
        throw std::invalid_argument(
            "ERROR: BP4Serializer::MakeHeader can only be called for an empty "
            "buffer. This one for " +
            fileType + " already has content of " + std::to_string(position) +
            " bytes.");
    }

    if (b.GetAvailableSize() < 64)
    {
        b.Resize(position + 64, "BP4Serializer::MakeHeader " + fileType);
    }

    const std::string majorVersion(std::to_string(ADIOS2_VERSION_MAJOR));
    const std::string minorVersion(std::to_string(ADIOS2_VERSION_MINOR));
    const std::string patchVersion(std::to_string(ADIOS2_VERSION_PATCH));

    // byte 0-31: Readable tag
    if (position != m_VersionTagPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Version Tag "
            "position mismatch");
    }
    std::string versionLongTag("ADIOS-BP v" + majorVersion + "." +
                               minorVersion + "." + patchVersion + " ");
    size_t maxTypeLen = m_VersionTagLength - versionLongTag.size();
    const std::string fileTypeStr = fileType.substr(0, maxTypeLen);
    versionLongTag += fileTypeStr;
    const size_t versionLongTagSize = versionLongTag.size();
    if (versionLongTagSize < m_VersionTagLength)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             versionLongTagSize);
        position += m_VersionTagLength - versionLongTagSize;
    }
    else if (versionLongTagSize > m_VersionTagLength)
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             m_VersionTagLength);
    }
    else
    {
        helper::CopyToBuffer(buffer, position, versionLongTag.c_str(),
                             m_VersionTagLength);
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
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Endian Flag "
            "position mismatch");
    }
    const uint8_t endianness = helper::IsLittleEndian() ? 0 : 1;
    helper::CopyToBuffer(buffer, position, &endianness);

    // byte 37: BP Version 4
    if (position != m_BPVersionPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Active Flag "
            "position mismatch");
    }
    const uint8_t version = 5;
    helper::CopyToBuffer(buffer, position, &version);

    // byte 38: Active flag (used in Index Table only)
    if (position != m_ActiveFlagPosition)
    {
        throw std::runtime_error(
            "ADIOS Coding ERROR in BP4Serializer::MakeHeader. Active Flag "
            "position mismatch");
    }
    const uint8_t activeFlag = (isActive ? 1 : 0);
    helper::CopyToBuffer(buffer, position, &activeFlag);

    // byte 39: Minor file version
    const uint8_t subversion = 0;
    helper::CopyToBuffer(buffer, position, &subversion);

    // bytes 40-43 writer count
    const uint32_t WriterCount = m_Comm.Size();
    helper::CopyToBuffer(buffer, position, &WriterCount);
    // bytes 44-47 aggregator count
    helper::CopyToBuffer(buffer, position, &WriterCount);
    // byte 48 columnMajor
    // write if data is column major in metadata and data
    const uint8_t columnMajor =
        (m_IO.m_ArrayOrder == ArrayOrdering::ColumnMajor) ? 'y' : 'n';
    helper::CopyToBuffer(buffer, position, &columnMajor);

    // byte 49-63: unused
    position += 15;
    absolutePosition = position;
}

void BP5Writer::InitBPBuffer()
{
    /* This is a new file.
     * Make headers in data buffer and metadata buffer (but do not write
     * them yet so that Open() can stay free of writing to disk)
     */

    const uint64_t a = static_cast<uint64_t>(m_Aggregator->m_SubStreamIndex);
    std::vector<uint64_t> Assignment = m_Comm.GatherValues(a, 0);

    if (m_Comm.Rank() == 0)
    {
        format::BufferSTL b;
        MakeHeader(b, "Metadata", false);
        m_FileMetadataManager.WriteFiles(b.m_Buffer.data(), b.m_Position);
        m_MetaDataPos = b.m_Position;
        format::BufferSTL bi;
        MakeHeader(bi, "Index Table", true);
        m_FileMetadataIndexManager.WriteFiles(bi.m_Buffer.data(),
                                              bi.m_Position);
        // where each rank's data will end up
        m_FileMetadataIndexManager.WriteFiles((char *)Assignment.data(),
                                              sizeof(Assignment[0]) *
                                                  Assignment.size());
    }

    if (m_Aggregator->m_IsAggregator)
    {
        m_DataPos = 0;
    }

    if (m_Comm.Rank() == 0)
    {
        m_WriterDataPos.resize(m_Comm.Size());
    }
}

void BP5Writer::NotifyEngineAttribute(std::string name, DataType type) noexcept
{
    m_MarshaledAttributesCount = 0;
}

void BP5Writer::FlushData(const bool isFinal)
{
    BufferV *DataBuf;
    if (m_Parameters.BufferVType == (int)BufferVType::MallocVType)
    {
        DataBuf = m_BP5Serializer.ReinitStepData(
            new MallocV("BP5Writer", false, m_Parameters.InitialBufferSize,
                        m_Parameters.GrowthFactor));
    }
    else
    {
        DataBuf = m_BP5Serializer.ReinitStepData(
            new ChunkV("BP5Writer", false /* always copy */,
                       m_Parameters.BufferChunkSize));
    }

    WriteData(DataBuf);

    m_ThisTimestepDataSize += DataBuf->Size();

    if (!isFinal)
    {
        size_t tmp[2];
        // aggregate start pos and data size to rank 0
        tmp[0] = m_StartDataPos;
        tmp[1] = DataBuf->Size();

        std::vector<size_t> RecvBuffer;
        if (m_Comm.Rank() == 0)
        {
            RecvBuffer.resize(m_Comm.Size() * 2);
        }
        m_Comm.GatherArrays(tmp, 2, RecvBuffer.data(), 0);
        if (m_Comm.Rank() == 0)
        {
            FlushPosSizeInfo.push_back(RecvBuffer);
        }
    }
    delete DataBuf;
}

void BP5Writer::Flush(const int transportIndex) { FlushData(false); }

void BP5Writer::DoClose(const int transportIndex)
{
    PERFSTUBS_SCOPED_TIMER("BP5Writer::Close");

    if ((m_WriterStep == 0) && !m_BetweenStepPairs)
    {
        /* never did begin step, do one now */
        BeginStep(StepMode::Update);
    }
    if (m_BetweenStepPairs)
    {
        EndStep();
    }
    m_FileDataManager.CloseFiles(transportIndex);
    // Delete files from temporary storage if draining was on

    if (m_Comm.Rank() == 0)
    {
        // close metadata file
        m_FileMetadataManager.CloseFiles();

        // close metametadata file
        m_FileMetaMetadataManager.CloseFiles();

        // close metadata index file
        m_FileMetadataIndexManager.CloseFiles();
    }

    FlushProfiler();
}

void BP5Writer::FlushProfiler()
{
    auto transportTypes = m_FileDataManager.GetTransportsTypes();

    // find first File type output, where we can write the profile
    int fileTransportIdx = -1;
    for (size_t i = 0; i < transportTypes.size(); ++i)
    {
        if (transportTypes[i].compare(0, 4, "File") == 0)
        {
            fileTransportIdx = static_cast<int>(i);
        }
    }

    auto transportProfilers = m_FileDataManager.GetTransportsProfilers();

    auto transportTypesMD = m_FileMetadataManager.GetTransportsTypes();
    auto transportProfilersMD = m_FileMetadataManager.GetTransportsProfilers();

    transportTypes.insert(transportTypes.end(), transportTypesMD.begin(),
                          transportTypesMD.end());

    transportProfilers.insert(transportProfilers.end(),
                              transportProfilersMD.begin(),
                              transportProfilersMD.end());

    // m_Profiler.WriteOut(transportTypes, transportProfilers);

    const std::string lineJSON(
        m_Profiler.GetRankProfilingJSON(transportTypes, transportProfilers) +
        ",\n");

    const std::vector<char> profilingJSON(
        m_Profiler.AggregateProfilingJSON(lineJSON));

    if (m_RankMPI == 0)
    {
        // std::cout << "write profiling file!" << std::endl;
        std::string profileFileName;
        if (m_DrainBB)
        {
            // auto bpTargetNames = m_BP4Serializer.GetBPBaseNames({m_Name});
            std::vector<std::string> bpTargetNames = {m_Name};
            if (fileTransportIdx > -1)
            {
                profileFileName =
                    bpTargetNames[fileTransportIdx] + "/profiling.json";
            }
            else
            {
                profileFileName = bpTargetNames[0] + "_profiling.json";
            }
            m_FileDrainer.AddOperationWrite(
                profileFileName, profilingJSON.size(), profilingJSON.data());
        }
        else
        {
            transport::FileFStream profilingJSONStream(m_Comm);
            // auto bpBaseNames = m_BP4Serializer.GetBPBaseNames({m_BBName});
            std::vector<std::string> bpBaseNames = {m_Name};
            if (fileTransportIdx > -1)
            {
                profileFileName =
                    bpBaseNames[fileTransportIdx] + "/profiling.json";
            }
            else
            {
                profileFileName = bpBaseNames[0] + "_profiling.json";
            }
            profilingJSONStream.Open(profileFileName, Mode::Write);
            profilingJSONStream.Write(profilingJSON.data(),
                                      profilingJSON.size());
            profilingJSONStream.Close();
        }
    }
}

/*write the content of metadata index file*/
void BP5Writer::PopulateMetadataIndexFileContent(
    format::BufferSTL &b, const uint64_t currentStep, const uint64_t mpirank,
    const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
    const uint64_t currentTimeStamp)
{
    PERFSTUBS_SCOPED_TIMER("BP5Writer::PopulateMetadataIndexFileContent");
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

} // end namespace engine
} // end namespace core
} // end namespace adios2
