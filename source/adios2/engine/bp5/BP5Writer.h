/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP5Writer.h
 *
 */

#ifndef ADIOS2_ENGINE_BP5_BP5WRITER_H_
#define ADIOS2_ENGINE_BP5_BP5WRITER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/engine/bp5/BP5Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/aggregator/mpi/MPIChain.h"
#include "adios2/toolkit/burstbuffer/FileDrainerSingleThread.h"
#include "adios2/toolkit/format/bp5/BP5Serializer.h"
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class BP5Writer : public BP5Engine, public core::Engine
{

public:
    /**
     * Constructor for file Writer in BP5 format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param comm multi-process communicator
     */
    BP5Writer(IO &io, const std::string &name, const Mode mode,
              helper::Comm comm);

    ~BP5Writer() = default;

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;

private:
    /** Single object controlling BP buffering */
    format::BP5Serializer m_BP5Serializer;

    /** Manage BP data files Transports from IO AddTransport */
    transportman::TransportMan m_FileDataManager;

    /** Manages the optional collective metadata files */
    transportman::TransportMan m_FileMetadataManager;

    /* transport manager for managing the metadata index file */
    transportman::TransportMan m_FileMetadataIndexManager;

    transportman::TransportMan m_FileMetaMetadataManager;

    int64_t m_WriterStep = -1;
    /*
     *  Burst buffer variables
     */
    /** true if burst buffer is used to write */
    bool m_WriteToBB = false;
    /** true if burst buffer is drained to disk  */
    bool m_DrainBB = true;
    /** File drainer thread if burst buffer is used */
    burstbuffer::FileDrainerSingleThread m_FileDrainer;
    /** m_Name modified with burst buffer path if BB is used,
     * == m_Name otherwise.
     * m_Name is a constant of Engine and is the user provided target path
     */
    std::string m_BBName;
    /* Name of subfiles to directly write to (for all transports)
     * This is either original target or burst buffer if used */
    std::vector<std::string> m_SubStreamNames;
    /* Name of subfiles on target if burst buffer is used (for all transports)
     */
    std::vector<std::string> m_DrainSubStreamNames;
    std::vector<std::string> m_MetadataFileNames;
    std::vector<std::string> m_DrainMetadataFileNames;
    std::vector<std::string> m_MetaMetadataFileNames;
    std::vector<std::string> m_MetadataIndexFileNames;
    std::vector<std::string> m_DrainMetadataIndexFileNames;
    std::vector<std::string> m_ActiveFlagFileNames;

    void Init() final;

    /** Parses parameters from IO SetParameters */
    void InitParameters() final;
    /** Parses transports and parameters from IO AddTransport */
    void InitTransports() final;
    /** Allocates memory and starts a PG group */
    void InitBPBuffer();

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutCommon(Variable<T> &variable, const T *data, bool sync);

    void DoFlush(const bool isFinal = false, const int transportIndex = -1);

    void DoClose(const int transportIndex = -1) final;

    /** Write a profiling.json file from m_BP1Writer and m_TransportsManager
     * profilers*/
    void WriteProfilingJSONFile();

    void WriteMetaMetadata(
        const std::vector<format::BP5Base::MetaMetaInfoBlock> MetaMetaBlocks);

    void WriteMetadataFileIndex(uint64_t MetaDataPos, uint64_t MetaDataSize);

    uint64_t
    WriteMetadata(const std::vector<format::BufferV::iovec> MetaDataBlocks,
                  const std::vector<format::BufferV::iovec> AttributeBlocks);

    /** Write Data to disk, in an aggregator chain */
    void WriteData(format::BufferV *Data);

    void PopulateMetadataIndexFileContent(
        format::BufferSTL &buffer, const uint64_t currentStep,
        const uint64_t mpirank, const uint64_t pgIndexStart,
        const uint64_t variablesIndexStart, const uint64_t attributesIndexStart,
        const uint64_t currentStepEndPos, const uint64_t currentTimeStamp);

    void UpdateActiveFlag(const bool active);

    void WriteCollectiveMetadataFile(const bool isFinal = false);

    void MarshalAttributes();

    /**
     * N-to-N data buffers writes, including metadata file
     * @param transportIndex
     */
    //    void WriteData(const bool isFinal, const int transportIndex = -1);

    /**
     * N-to-M (aggregation) data buffers writes, including metadata file
     * @param transportIndex
     */
    void AggregateWriteData(const bool isFinal, const int transportIndex = -1);

    template <class T>
    T *BufferDataCommon(const size_t payloadOffset,
                        const size_t bufferID) noexcept;

    template <class T>
    void PerformPutCommon(Variable<T> &variable);

    /** manages all communication tasks in aggregation */
    aggregator::MPIChain m_Aggregator;

private:
    // updated during WriteMetaData
    uint64_t m_MetaDataPos = 0;

    /** On every process, at the end of writing, this holds the offset
     *  where they started writing (needed for global metadata)
     */
    uint64_t m_StartDataPos = 0;
    /** On aggregators, at the end of writing, this holds the starting offset
     *  to the next step's writing; otherwise used as temporary offset variable
     *  during writing on every process and points to the end of the process'
     *  data block in the file (not used for anything)
     */
    uint64_t m_DataPos = 0;

    /** rank 0 collects m_StartDataPos in this vector for writing it
     *  to the index file
     */
    std::vector<uint64_t> m_WriterDataPos;

    uint32_t m_MarshaledAttributesCount =
        0; // updated during EndStep/MarshalAttributes

    // where each writer rank writes its data, init in InitBPBuffer;
    std::vector<uint64_t> m_Assignment;

    void MakeHeader(format::BufferSTL &b, const std::string fileType,
                    const bool isActive);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP5_BP5WRITER_H_ */
