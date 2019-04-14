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

#include "adios2/ADIOSConfig.h"
#include "adios2/core/Engine.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.h"
#include "adios2/toolkit/format/dataman/DataManSerializer.tcc"
#include "adios2/toolkit/transportman/stagingman/StagingMan.h"

namespace adios2
{
namespace core
{
namespace engine
{

class TableWriter : public Engine
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
    TableWriter(IO &adios, const std::string &name, const Mode mode,
                   MPI_Comm mpiComm);

    ~TableWriter() = default;

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const final;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    int m_Verbosity = 0;
    int m_CurrentStep = -1;
    int m_MpiRank;
    int m_MpiSize;
    int m_Timeout = 5;
    size_t m_RowsPerRank = 128;
    size_t m_AppID;
    std::unordered_map<std::string, std::unordered_map<size_t, std::vector<char>>> m_AggregatorBuffers;
    std::unordered_map<std::string, std::pair<size_t, size_t>> m_AggregatorRecord;
    std::unordered_map<int, std::string> m_AllAddresses;
    int m_Port = 6789;
    int m_MaxRanksPerNode = 200;
    bool m_Listening;
    std::thread m_ReplyThread;
    bool m_FirstPut = true;

    void Init() final;
    void InitParameters() final;
    void InitTransports() final;
    void ReplyThread();
    void CheckFlush();

    std::vector<int> WhichRanks(const Dims &start, const Dims &count);
    int WhichRank(size_t row);

    format::DataManSerializer m_DataManSerializer;
    transportman::StagingMan m_SendStagingMan;

#define declare_type(T)                                                        \
    void DoPutSync(Variable<T> &, const T *) final;                            \
    void DoPutDeferred(Variable<T> &, const T *) final;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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

#endif /* ADIOS2_ENGINE_TABLEWRITER_H_ */
