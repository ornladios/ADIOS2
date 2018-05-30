/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataMan.h
 *
 *  Created on: Jan 10, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_

#include "DataManCommon.h"

namespace adios2
{

class DataManWriter : public DataManCommon
{

public:
    DataManWriter(IO &io, const std::string &name, const Mode mode,
                  MPI_Comm mpiComm);

    ~DataManWriter() = default;

    StepStatus BeginStep(StepMode mode, const float timeoutSeconds = 0.f) final;
    void EndStep() final;
    size_t CurrentStep() const;

private:
    unsigned int m_nDataThreads = 1;
    unsigned int m_nControlThreads = 0;
    unsigned int m_TransportChannels = 1;
    size_t m_BufferSize = 1024 * 1024 * 1024;
    bool m_DoMonitor = false;
    bool m_Blocking = true;
    size_t m_StepsPerBuffer = 10;

    std::shared_ptr<format::BP3Serializer> m_BP3Serializer;
    std::shared_ptr<format::DataManSerializer> m_DataManSerializer;
    std::string m_Name;

    void Init();
    void IOThread(std::shared_ptr<transportman::DataMan> man) final;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutSyncCommonBP(Variable<T> &variable, const T *values);

    template <class T>
    void PutSyncCommonDataMan(Variable<T> &variable, const T *values);

    void DoClose(const int transportIndex = -1) final;
};

} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
