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
#include "adios2/toolkit/format/bp1/BP1.h"            //format::BP1Writer
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
    BPFileWriter(IO &io, const std::string &name, const OpenMode openMode,
                 MPI_Comm mpiComm);

    ~BPFileWriter();

    void Advance(const float timeoutSeconds = 0.0) final;

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports, otherwise
     * it
     * closes a transport in m_Transport[transportIndex]. In debug mode the
     * latter
     * is bounds-checked.
     */
    void Close(const int transportIndex = -1) final;

private:
    /** Single object controlling BP buffering */
    format::BP1Writer m_BP1Writer;

    /** single object controlling a vector of Transports from IO AddTransport */
    transportman::TransportMan m_TransportsManager;

    /** true: due to buffer overflow, move to transports manager */
    bool m_DoTransportFlush = false;

    void Init() final;

    /** Parses parameters from IO SetParameters */
    void InitParameters() final;
    /** Parses transports and parameters from IO AddTransport */
    void InitTransports() final;

    void InitBPBuffer();

#define declare_type(T)                                                        \
    void DoWrite(Variable<T> &variable, const T *values) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Common function for primitive (including std::complex) writes
     * @param variable
     * @param values
     */
    template <class T>
    void DoWriteCommon(Variable<T> &variable, const T *values);

    /** Write a profiling.json file from m_BP1Writer and m_TransportsManager
     * profilers*/
    void WriteProfilingJSONFile();
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_BP_BPFILEWRITER_H_ */
