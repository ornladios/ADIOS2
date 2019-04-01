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

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp4/BP4.h"
#include "adios2/toolkit/transportman/TransportMan.h" //transport::TransportsMan

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
     * @param mpiComm MPI communicator
     */
    BP4Writer(IO &io, const std::string &name, const Mode mode,
              MPI_Comm mpiComm);

    ~BP4Writer();

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
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;

    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
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

    void PopulateMetadataIndexFileHeader(std::vector<char> &buffer,
                                         size_t &position, const uint8_t,
                                         const bool addSubfiles);

    void PopulateMetadataIndexFileContent(
        const uint64_t currentStep, const uint64_t mpirank,
        const uint64_t pgIndexStart, const uint64_t variablesIndexStart,
        const uint64_t attributesIndexStart, const uint64_t currentStepEndPos,
        std::vector<char> &buffer, size_t &position);

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
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4WRITER_H_ */
