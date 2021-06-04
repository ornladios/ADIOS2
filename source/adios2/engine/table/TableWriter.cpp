/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#include "TableWriter.h"
#include "TableWriter.tcc"

#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"

#include <iostream>

namespace adios2
{
namespace core
{
namespace engine
{

TableWriter::TableWriter(IO &io, const std::string &name, const Mode mode,
                         helper::Comm comm)
: Engine("TableWriter", io, name, mode, std::move(comm)),
  m_SubAdios(m_Comm.World(), "C++"), m_SubIO(m_SubAdios.DeclareIO("SubIO")),
  m_SzOperator(compress::CompressSZ(Params()))
{
    m_MpiRank = m_Comm.Rank();
    m_MpiSize = m_Comm.Size();

    m_SubEngine = &m_SubIO.Open(m_Name, adios2::Mode::Write);
}

TableWriter::~TableWriter() {}

StepStatus TableWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    return m_SubEngine->BeginStep(mode, timeoutSeconds);
}

size_t TableWriter::CurrentStep() const { return 0; }

void TableWriter::PerformPuts() { m_SubEngine->PerformPuts(); }

void TableWriter::EndStep() { m_SubEngine->EndStep(); }

void TableWriter::Flush(const int transportIndex)
{
    m_SubEngine->Flush(transportIndex);
}

// PRIVATE

#define declare_type(T)                                                        \
    void TableWriter::DoPutSync(Variable<T> &variable, const T *data)          \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
    }                                                                          \
    void TableWriter::DoPutDeferred(Variable<T> &variable, const T *data)      \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void TableWriter::DoClose(const int transportIndex)
{
    m_SubEngine->Close();
    m_SubEngine = nullptr;
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
