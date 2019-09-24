/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Writer.h
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP4_BP4WRITER_H_
#define ADIOS2_ENGINE_BP4_BP4WRITER_H_

#include "adios2/common/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/format/bp/bp4/BP4Serializer.h"
#include "adios2/toolkit/transportman/TransportMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class BP4Writer : public core::Engine
{

public:
    /**
     * Constructor for file Writer in BP4 format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param comm multi-process communicator
     */
    BP4Writer(IO &io, const std::string &name, const Mode mode,
              helper::Comm comm);

    ~BP4Writer() = default;

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    /** Single object controlling BP buffering */
    format::BP4Serializer m_BP4Serializer;

    /** Manage BP data files Transports from IO AddTransport */
    transportman::TransportMan m_FileDataManager;

    /** future returned by m_FileDataManager at OpenFiles */
    std::future<void> m_FutureOpenFiles;

    /** Manages the optional collective metadata files */
    transportman::TransportMan m_FileMetadataManager;

    /* transport manager for managing the metadata index file */
    transportman::TransportMan m_FileMetadataIndexManager;

    void Init() final;

    /** Parses parameters from IO SetParameters */
    void InitParameters() final;
    /** Parses transports and parameters from IO AddTransport */
    void InitTransports() final;
    /** Allocates memory and starts a PG group */
    void InitBPBuffer();

#define declare_type(T)                                                        \
    void DoPut(Variable<T> &variable, typename Variable<T>::Span &span,        \
               const size_t bufferID, const T &value) final;

    ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutCommon(Variable<T> &variable, typename Variable<T>::Span &span,
                   const size_t bufferID, const T &value);

    template <class T>
    void PutSyncCommon(Variable<T> &variable,
                       const typename Variable<T>::Info &blockInfo);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *data);

    void DoFlush(const bool isFinal = false, const int transportIndex = -1);

    void DoClose(const int transportIndex = -1) final;

    /** Write a profiling.json file from m_BP1Writer and m_TransportsManager
     * profilers*/
    void WriteProfilingJSONFile();

    void UpdateActiveFlag(const bool active);

    void PopulateMetadataIndexFileContent(
        format::BufferSTL &buffer, const uint64_t currentStep,
        const uint64_t mpirank, const uint64_t pgIndexStart,
        const uint64_t variablesIndexStart, const uint64_t attributesIndexStart,
        const uint64_t currentStepEndPos, const uint64_t currentTimeStamp);

    void WriteCollectiveMetadataFile(const bool isFinal = false);

    /**
     * N-to-N data buffers writes, including metadata file
     * @param transportIndex
     */
    void WriteData(const bool isFinal, const int transportIndex = -1);

    /**
     * N-to-M (aggregation) data buffers writes, including metadata file
     * @param transportIndex
     */
    void AggregateWriteData(const bool isFinal, const int transportIndex = -1);

#define declare_type(T, L)                                                     \
    T *DoBufferData_##L(const size_t payloadPosition,                          \
                        const size_t bufferID = 0) noexcept final;

    ADIOS2_FOREACH_PRIMITVE_STDTYPE_2ARGS(declare_type)
#undef declare_type

    template <class T>
    T *BufferDataCommon(const size_t payloadOffset,
                        const size_t bufferID) noexcept;

    template <class T>
    void PerformPutCommon(Variable<T> &variable);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4WRITER_H_ */
