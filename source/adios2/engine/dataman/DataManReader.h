/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManReader.h
 *
 *  Created on: Feb 21, 2017
 *      Author: Jason Wang
 *              William F Godoy
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_

#include "DataManCommon.h"
#include "adios2/toolkit/format/dataman/DataManDeserializer.tcc"

namespace adios2
{
namespace core
{
namespace engine
{

class DataManReader : public DataManCommon
{

public:
    /**
     * Constructor for dataman engine Reader for WAN communications
     * @param adios
     * @param name
     * @param accessMode
     * @param mpiComm
     * @param method
     * @param debugMode
     * @param nthreads
     */
    DataManReader(IO &io, const std::string &name, const Mode mode,
                  MPI_Comm mpiComm);
    virtual ~DataManReader();
    StepStatus BeginStep(StepMode stepMode,
                         const float timeoutSeconds = 0.f) final;
    void PerformGets() final;
    void EndStep() final;
    size_t CurrentStep() const;

private:
    std::vector<core::Operator *> m_Callbacks;

    format::DataManDeserializer m_DataManDeserializer;

    bool m_Listening = false;

    std::mutex m_MutexIO;

    void IOThread(std::shared_ptr<transportman::DataMan> man) final;
    void IOThreadBP(std::shared_ptr<transportman::DataMan> man);

    void Init();

    void RunCallback();

    void DoClose(const int transportIndex = -1) final;

#define declare_type(T)                                                        \
    void DoGetSync(Variable<T> &, T *) final;                                  \
    void DoGetDeferred(Variable<T> &, T *) final;
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * All GetSync virtual functions call this function
     * @param variable
     * @param data
     */
    template <class T>
    void GetSyncCommon(Variable<T> &variable, T *data);

    template <class T>
    void GetDeferredCommon(Variable<T> &variable, T *data);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANREADER_H_ */
