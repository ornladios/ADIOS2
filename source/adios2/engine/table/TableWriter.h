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

#include "adios2/core/ADIOS.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"

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
    int m_MpiRank;
    int m_MpiSize;
    ADIOS m_SubAdios;
    IO &m_SubIO;
    Engine *m_SubEngine = nullptr;
    Operator *m_Compressor = nullptr;
    std::unordered_map<std::string, bool> m_IndexerMap;
    std::string m_UseCompressor;
    std::string m_UseAccuracy;

    void PutSubEngine(bool finalPut = false);

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
