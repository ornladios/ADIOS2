/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPFileWriter.h
 *
 *  Created on: Dec 16, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP_BPFILEWRITER_H_
#define ADIOS2_ENGINE_BP_BPFILEWRITER_H_

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/bp3/BP3.h"
#include "adios2/toolkit/transportman/TransportMan.h" //transport::TransportsMan

namespace adios2
{

class BPFileWriter : public Engine
{

public:
    /**
     * Constructor for file Writer in BP format
     * @param name unique name given to the engine
     * @param openMode w (supported), r, a from OpenMode in ADIOSTypes.h
     * @param mpiComm MPI communicator
     */
    BPFileWriter(IO &io, const std::string &name, const Mode mode,
                 MPI_Comm mpiComm);

    ~BPFileWriter();

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    /** Single object controlling BP buffering */
    format::BP3Serializer m_BP3Serializer;

    /** Manage BP data files Transports from IO AddTransport */
    transportman::TransportMan m_FileDataManager;

    /** Manages the optional collective metadata files */
    transportman::TransportMan m_FileMetadataManager;

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

    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    void DoFlush(const bool isFinal = false, const int transportIndex = -1);

    void DoClose(const int transportIndex = -1) final;

    /** Write a profiling.json file from m_BP1Writer and m_TransportsManager
     * profilers*/
    void WriteProfilingJSONFile();

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

} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP_BPFILEWRITER_H_ */
