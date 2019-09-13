/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManWriter.h
 *
 *  Created on: Jan 10, 2017
 *      Author: Jason Wang
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMANWRITER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMANWRITER_H_

#include "DataManCommon.h"

namespace adios2
{
namespace core
{
namespace engine
{

class DataManWriter : public DataManCommon
{

public:
    DataManWriter(IO &io, const std::string &name, const Mode mode,
                  helper::Comm comm);
    virtual ~DataManWriter();

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    std::string m_DataAddress;
    std::string m_ControlAddress;
    std::string m_AllAddresses;

    adios2::zmq::ZmqPubSub m_DataPublisher;

    void ReplyThread(const std::string &address);
    std::thread m_ReplyThread;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);

    void DoClose(const int transportIndex = -1) final;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMANWRITER_H_ */
