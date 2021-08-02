/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * TableWriter.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#include "TableWriter.tcc"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

TableWriter::TableWriter(IO &io, const std::string &name, const Mode mode,
                         helper::Comm comm)
: Engine("TableWriter", io, name, mode, std::move(comm)),
  m_SubAdios(m_Comm.Duplicate(), io.m_HostLanguage),
  m_SubIO(m_SubAdios.DeclareIO("SubIO"))
{
    helper::GetParameter(io.m_Parameters, "tiers", m_Tiers);
    for (const auto &params : io.m_TransportsParameters)
    {
        auto it = params.find("variable");
        if (it == params.end())
        {
            continue;
        }
        for (const auto &param : params)
        {
            m_OperatorMap[it->second][param.first] = param.second;
        }
    }
    for (int i = 0; i < m_Tiers; ++i)
    {
        m_SubEngines.push_back(&m_SubIO.Open(
            m_Name + ".tier" + std::to_string(i), adios2::Mode::Write));
    }
}

TableWriter::~TableWriter()
{
    for (auto &c : m_Compressors)
        if (c)
        {
            delete c;
            c = nullptr;
        }
}

StepStatus TableWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{

    for (auto &e : m_SubEngines)
    {
        e->BeginStep(mode, timeoutSeconds);
    }
    return StepStatus::OK;
}

size_t TableWriter::CurrentStep() const
{
    return m_SubEngines[0]->CurrentStep();
}

void TableWriter::PerformPuts()
{
    for (auto &e : m_SubEngines)
    {
        e->PerformPuts();
    }
}

void TableWriter::EndStep()
{
    for (auto &e : m_SubEngines)
    {
        e->EndStep();
    }
}

void TableWriter::Flush(const int transportIndex)
{
    for (auto &e : m_SubEngines)
    {
        e->Flush(transportIndex);
    }
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
    for (auto &e : m_SubEngines)
    {
        e->Close();
        e = nullptr;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
