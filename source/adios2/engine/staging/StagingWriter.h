/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * StagingWriter.h
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_STAGINGWRITER_H_
#define ADIOS2_ENGINE_STAGINGWRITER_H_

#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/transportman/stagingman/StagingMan.h"
#include "adios2/toolkit/transportman/wanman/WANMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class StagingWriter : public Engine
{

public:
    /**
     * Constructor for Writer
     * @param name unique name given to the engine
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     */
    StagingWriter(IO &adios, const std::string &name, const Mode mode,
                  MPI_Comm mpiComm);

    ~StagingWriter() = default;

    StepStatus BeginStep(
        StepMode mode,
        const float timeoutSeconds = std::numeric_limits<float>::max()) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    format::DataManSerializer m_DataManSerializer;
    transportman::WANMan m_MetadataTransport;
    int m_Verbosity = 0;
    int64_t m_CurrentStep = -1;
    int m_MpiRank;
    int m_MpiSize;
    std::string m_IP = "127.0.0.1";
    std::string m_MetadataPort;
    std::string m_FullDataAddress;
    int m_Timeout = 5;
    bool m_Listening = false;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    void Handshake();
    void IOThread();
    std::thread m_IOThread;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Closes a single transport or all transports
     * @param transportIndex, if -1 (default) closes all transports,
     * otherwise it closes a transport in m_Transport[transportIndex].
     * In debug mode the latter is bounds-checked.
     */
    void DoClose(const int transportIndex = -1) final;

    /**
     * Common function for primitive PutSync, puts variables in buffer
     * @param variable
     * @param values
     */
    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_STAGINGWRITER_H_
