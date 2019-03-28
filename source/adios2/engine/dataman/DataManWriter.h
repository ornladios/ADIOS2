/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataManWriter.h
 *
 *  Created on: Jan 10, 2017
 *      Author: Jason Wang
 *              William F Godoy
 */

#ifndef ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_
#define ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_

#include "DataManCommon.h"
#include "adios2/toolkit/transportman/stagingman/StagingMan.h"

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
                  MPI_Comm mpiComm);
    ~DataManWriter() = default;

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) final;
    size_t CurrentStep() const;
    void PerformPuts() final;
    void EndStep() final;
    void Flush(const int transportIndex = -1) final;

private:
    size_t m_BufferSize = 1024 * 1024 * 1024;
    bool m_Listening = true;
    format::VecPtr m_AggregatedMetadata = nullptr;
    std::mutex m_AggregatedMetadataMutex;
    int m_AppID = 0;
    int m_Port = 12307;
    std::vector<std::string> m_FullAddresses;

    std::vector<std::shared_ptr<format::DataManSerializer>> m_DataManSerializer;

    void Init();
    void MetadataThread(const std::string &address);
    std::thread m_MetadataThread;

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

#endif /* ADIOS2_ENGINE_DATAMAN_DATAMAN_WRITER_H_ */
