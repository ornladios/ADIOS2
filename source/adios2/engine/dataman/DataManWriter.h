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

#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/zmq/zmqpubsub/ZmqPubSub.h"
#include "adios2/toolkit/zmq/zmqreqrep/ZmqReqRep.h"

namespace adios2
{
namespace core
{
namespace engine
{

class DataManWriter : public Engine
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
    int m_CurrentReaderCount = 0;
    int m_MpiRank;
    int m_MpiSize;
    std::string m_IPAddress;
    int m_Port = 50001;
    int m_RendezvousReaderCount = 1;
    int m_RendezvousMilliseconds = 1000;
    int m_Timeout = 5;
    int m_Verbosity = 0;
    int64_t m_CurrentStep = -1;
    size_t m_SerializerBufferSize = 128 * 1024 * 1024;
    bool m_ThreadActive = true;

    format::DataManSerializer m_Serializer;
    adios2::zmq::ZmqPubSub m_DataPublisher;

    void ReplyThread(const std::string &address, const int times);
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
