/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.h
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#ifndef ADIOS2_ENGINE_TABLEWRITER_H_
#define ADIOS2_ENGINE_TABLEWRITER_H_

#include "adios2/core/Engine.h"
#include "adios2/engine/bp3/BP3Writer.h"
#include "adios2/engine/bp4/BP4Writer.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/zmq/zmqreqrep/ZmqReqRep.h"

#include "../../bindings/CXX11/adios2.h"

namespace adios2
{
namespace core
{
namespace engine
{

class TableWriter : public Engine
{

public:
    TableWriter(IO &adios, const std::string &name, const Mode mode,
                helper::Comm comm);

    virtual ~TableWriter();

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    struct VarInfo
    {
        Dims shape;
        std::string type;
    };
    int m_Verbosity = 0;
    int m_Timeout = 5;
    int m_Port = 6789;
    int m_MaxRanksPerNode = 200;
    int m_Aggregators = 10;
    size_t m_SerializerBufferSize = 1 * 1024 * 1024;
    size_t m_ReceiverBufferSize = 512 * 1024 * 1024;
    size_t m_RowsPerAggregatorBuffer = 400;
    std::unordered_map<size_t,
                       std::unordered_map<std::string, std::vector<char>>>
        m_AggregatorBuffers;
    std::unordered_map<size_t,
                       std::unordered_map<std::string, std::vector<bool>>>
        m_AggregatorBufferFlags;
    std::unordered_map<std::string, VarInfo> m_VarInfoMap;
    std::unordered_map<int, std::string> m_AllAddresses;
    bool m_Listening;
    int m_MpiRank;
    int m_MpiSize;
    int m_CurrentStep = -1;
    size_t m_AppID;
    std::thread m_ReplyThread;
    bool m_IsRowMajor;
    adios2::ADIOS m_SubAdios;
    adios2::IO m_SubIO;
    std::shared_ptr<adios2::Engine> m_SubEngine;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    void ReplyThread();
    void PutSubEngine(bool finalPut = false);
    void PutAggregatorBuffer();

    std::vector<int> WhatAggregatorIndices(const Dims &start,
                                           const Dims &count);
    std::vector<std::string> WhatAggregatorAddresses(const Dims &start,
                                                     const Dims &count);
    std::vector<std::string>
    WhatAggregatorAddresses(const std::vector<int> &indices);

    std::vector<size_t> WhatBufferIndices(const Dims &start, const Dims &count);
    size_t WhatBufferIndex(const size_t row);

    Dims WhatStart(const Dims &shape, const size_t index);
    Dims WhatCount(const Dims &shape, const size_t index);

    std::vector<std::shared_ptr<format::DataManSerializer>> m_Serializers;
    format::DataManSerializer m_Deserializer;
    adios2::zmq::ZmqReqRep m_SendStagingMan;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) final;

    template <class T>
    void PutSyncCommon(Variable<T> &variable, const T *values);

    template <class T>
    void PutDeferredCommon(Variable<T> &variable, const T *values);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif // ADIOS2_ENGINE_TABLEWRITER_H_
