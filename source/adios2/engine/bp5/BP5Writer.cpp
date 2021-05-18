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

BP5Writer::BP5Writer(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("BP5Writer", io, name, mode, std::move(comm)), m_BP5Serializer(),
  m_FileDataManager(m_Comm), m_FileMetadataManager(m_Comm),
  m_FileMetaMetadataManager(m_Comm), m_FileMetadataIndexManager(m_Comm)
{
    TAU_SCOPED_TIMER("BP5Writer::Open");
    m_IO.m_ReadStreaming = false;
    m_EndMessage = " in call to IO Open BP5Writer " + m_Name + "\n";

    Init();
}

StepStatus BP5Writer::BeginStep(StepMode mode, const float timeoutSeconds)
{
    m_WriterStep++;
    return StepStatus::OK;
}

size_t BP5Writer::CurrentStep() const { return m_WriterStep; }

void BP5Writer::PerformPuts()
{
    TAU_SCOPED_TIMER("BP5Writer::PerformPuts");
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

uint64_t BP5Writer::WriteMetadata(const std::vector<iovec> MetaDataBlocks)
{
    uint64_t MDataTotalSize = 0;
    uint64_t MetaDataSize = 0;
    std::vector<uint64_t> SizeVector;
    SizeVector.reserve(MetaDataBlocks.size());
    for (auto &b : MetaDataBlocks)
    {
        MDataTotalSize += sizeof(uint64_t) + b.iov_len;
        SizeVector.push_back(b.iov_len);
    }
    MetaDataSize = 0;
    m_FileMetadataManager.WriteFiles((char *)&MDataTotalSize, sizeof(uint64_t));
    MetaDataSize += sizeof(uint64_t);
    m_FileMetadataManager.WriteFiles((char *)SizeVector.data(),
                                     sizeof(uint64_t) * SizeVector.size());
    MetaDataSize += sizeof(uint64_t) * SizeVector.size();
    for (auto &b : MetaDataBlocks)
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
    format::BufferV::BufferV_iovec DataVec = Data->DataVec();
    size_t DataSize = 0;
    int i = 0;
    while (DataVec[i].iov_base != NULL)
    {
        m_FileDataManager.WriteFiles((char *)DataVec[i].iov_base,
                                     DataVec[i].iov_len);
        DataSize += DataVec[i].iov_len;
        i++;
    }
    std::cout << "before update m_DataPos is " << m_DataPos << std::endl;
    m_DataPos += DataSize;
}

void BP5Writer::WriteMetadataFileIndex(uint64_t MetaDataPos,
                                       uint64_t MetaDataSize,
                                       std::vector<uint64_t> DataSizes)
{

    m_FileMetadataManager.FlushFiles();

    uint64_t buf[2];
    buf[0] = MetaDataPos;
    buf[1] = MetaDataSize;
    m_FileMetadataIndexManager.WriteFiles((char *)buf, sizeof(buf));
    for (int i = 0; i < DataSizes.size(); i++)
    {
        std::cout << "Writer data pos rank " << i << " = " << m_WriterDataPos[i]
                  << std::endl;
    }
    m_FileMetadataIndexManager.WriteFiles((char *)m_WriterDataPos.data(),
                                          DataSizes.size() * sizeof(uint64_t));
    for (int i = 0; i < DataSizes.size(); i++)
    {
        m_WriterDataPos[i] += DataSizes[i];
    }
}

void BP5Writer::EndStep()
{
    TAU_SCOPED_TIMER("BP5Writer::EndStep");

    // true: advances step
    auto TSInfo = m_BP5Serializer.CloseTimestep(m_WriterStep);

    /* TSInfo includes NewMetaMetaBlocks, the MetaEncodeBuffer, the
     * AttributeEncodeBuffer and the data encode Vector */
    /* the first */

    std::cout << "Endstp, data buffer size = " << TSInfo.DataBuffer->Size()
              << std::endl;
    std::vector<char> MetaBuffer = m_BP5Serializer.CopyMetadataToContiguous(
        TSInfo.NewMetaMetaBlocks, TSInfo.MetaEncodeBuffer,
        TSInfo.DataBuffer->Size());

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
    m_Comm.GathervArrays(MetaBuffer.data(), LocalSize, RecvCounts.data(),
                         RecvCounts.size(), RecvBuffer->data(), 0);

    if (m_Comm.Rank() == 0)
    {
        std::vector<format::BP5Base::MetaMetaInfoBlock> UniqueMetaMetaBlocks;
        std::vector<uint64_t> DataSizes;
        auto Metadata = m_BP5Serializer.BreakoutContiguousMetadata(
            RecvBuffer, RecvCounts, UniqueMetaMetaBlocks, DataSizes);
        std::cout << "Data sizes size " << DataSizes.size() << std::endl;
        WriteMetaMetadata(UniqueMetaMetaBlocks);
        uint64_t ThisMetaDataPos = m_MetaDataPos;
        uint64_t ThisMetaDataSize = WriteMetadata(Metadata);
        WriteMetadataFileIndex(ThisMetaDataPos, ThisMetaDataSize, DataSizes);
    }
    delete RecvBuffer;
    WriteData(TSInfo.DataBuffer);
}

// PRIVATE
void BP5Writer::Init()
{
    m_BP5Serializer.m_Engine = this;
    m_RankMPI = m_Comm.Rank();
    InitParameters();
    InitTransports();
    InitBPBuffer();
}

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

void BP5Writer::InitParameters()
{
    ParseParams(m_IO, m_Parameters);
    m_WriteToBB = false; // !(m_Parameters.BurstBufferPath.empty());
    m_DrainBB = m_WriteToBB && m_Parameters.BurstBufferDrain;
}

void BP5Writer::InitTransports()
{
    // TODO need to add support for aggregators here later
    if (m_IO.m_TransportsParameters.empty())
    {
        Params defaultTransportParameters;
        defaultTransportParameters["transport"] = "File";
        m_IO.m_TransportsParameters.push_back(defaultTransportParameters);
    }

    // only consumers will interact with transport managers
    m_BBName = m_Name;
    if (m_WriteToBB)
    {
        m_BBName = m_Parameters.BurstBufferPath + PathSeparator + m_Name;
    }

    if (m_Aggregator.m_IsConsumer)
    {
        // Names passed to IO AddTransport option with key "Name"
        const std::vector<std::string> transportsNames =
            m_FileDataManager.GetFilesBaseNames(m_BBName,
                                                m_IO.m_TransportsParameters);

        // /path/name.bp.dir/name.bp.rank
        m_SubStreamNames = GetBPSubStreamNames(transportsNames);
        if (m_DrainBB)
        {
            const std::vector<std::string> drainTransportNames =
                m_FileDataManager.GetFilesBaseNames(
                    m_Name, m_IO.m_TransportsParameters);
            m_DrainSubStreamNames = GetBPSubStreamNames(drainTransportNames);
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
        const std::vector<std::string> transportsNames =
            m_FileMetadataManager.GetFilesBaseNames(
                m_Name, m_IO.m_TransportsParameters);

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

    if (m_Aggregator.m_IsConsumer)
    {
#ifdef NOTDEF
        if (m_Parameters.AsyncTasks)
        {
            for (size_t i = 0; i < m_IO.m_TransportsParameters.size(); ++i)
            {
                m_IO.m_TransportsParameters[i]["asynctasks"] = "true";
            }
        }
#endif
        m_FileDataManager.OpenFiles(m_SubStreamNames, m_OpenMode,
                                    m_IO.m_TransportsParameters, false);

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
                                            m_IO.m_TransportsParameters, false);

        m_FileMetadataManager.OpenFiles(m_MetadataFileNames, m_OpenMode,
                                        m_IO.m_TransportsParameters, false);

        uint64_t WriterCount = m_Comm.Size();
        m_FileMetadataIndexManager.OpenFiles(
            m_MetadataIndexFileNames, m_OpenMode, m_IO.m_TransportsParameters,
            false);

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
    helper::CopyToBuffer(buffer, position, &version);

    // bytes 40-43 writer count
    const uint32_t WriterCount = m_Comm.Size();
    helper::CopyToBuffer(buffer, position, &WriterCount);
    // bytes 44-47 aggregator count
    helper::CopyToBuffer(buffer, position, &WriterCount);
    // byte 48 columnMajor
    // write if data is column major in metadata and data
    const uint8_t columnMajor =
        (helper::IsRowMajor(m_IO.m_HostLanguage) == false) ? 'y' : 'n';
    helper::CopyToBuffer(buffer, position, &columnMajor);

    // byte 45-63: unused
    position += 15;
    absolutePosition = position;
}

void BP5Writer::InitBPBuffer()
{
    /* This is a new file.
     * Make headers in data buffer and metadata buffer (but do not write
     * them yet so that Open() can stay free of writing to disk)
     */
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
        std::vector<uint64_t> Assignment(m_Comm.Size());
        for (uint64_t i = 0; i < m_Comm.Size(); i++)
        {
            Assignment[i] = i; // Change when we do aggregation
        }
        // where each rank's data will end up
        m_FileMetadataIndexManager.WriteFiles((char *)Assignment.data(),
                                              sizeof(Assignment[0]) *
                                                  Assignment.size());
    }
    if (m_Aggregator.m_IsConsumer)
    {
        format::BufferSTL d;
        MakeHeader(d, "Data", false);
        m_FileDataManager.WriteFiles(d.m_Buffer.data(), d.m_Position);
        m_DataPos = d.m_Position;
        m_WriterDataPos.resize(m_Comm.Size());
        for (auto &DataPos : m_WriterDataPos)
        {
            DataPos = m_DataPos;
        }
    }
}

void BP5Writer::DoFlush(const bool isFinal, const int transportIndex)
{
    m_FileMetadataManager.FlushFiles();
    m_FileMetaMetadataManager.FlushFiles();
    m_FileDataManager.FlushFiles();
    //    m_BP4Serializer.ResetBuffer(m_BP4Serializer.m_Data, false, false);

    //    if (m_Parameters.CollectiveMetadata)
    //    {
    //        WriteCollectiveMetadataFile();
    //    }
    //    if (m_BP4Serializer.m_Aggregator.m_IsActive)
    //    {
    //        AggregateWriteData(isFinal, transportIndex);
    //    }
    //    else
    //    {
    //        WriteData(isFinal, transportIndex);
    //    }
}

void BP5Writer::DoClose(const int transportIndex)
{
    TAU_SCOPED_TIMER("BP5Writer::Close");
    PerformPuts();

    DoFlush(true, transportIndex);

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
}

/*write the content of metadata index file*/
void BP5Writer::PopulateMetadataIndexFileContent(
    format::BufferSTL &b, const uint64_t currentStep, const uint64_t mpirank,
    const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
    const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
    const uint64_t currentTimeStamp)
{
    TAU_SCOPED_TIMER("BP5Writer::PopulateMetadataIndexFileContent");
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
