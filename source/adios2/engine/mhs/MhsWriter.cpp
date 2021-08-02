/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * MhsWriter.cpp
 *
 *  Created on: Apr 6, 2019
 *      Author: Jason Wang w4g@ornl.gov
 */

#include "MhsWriter.tcc"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

MhsWriter::MhsWriter(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("MhsWriter", io, name, mode, std::move(comm)),
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

MhsWriter::~MhsWriter()
{
    for (auto &c : m_Compressors)
        if (c)
        {
            delete c;
            c = nullptr;
        }
}

StepStatus MhsWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{

    for (auto &e : m_SubEngines)
    {
        e->BeginStep(mode, timeoutSeconds);
    }
    return StepStatus::OK;
}

size_t MhsWriter::CurrentStep() const { return m_SubEngines[0]->CurrentStep(); }

void MhsWriter::PerformPuts()
{
    for (auto &e : m_SubEngines)
    {
        e->PerformPuts();
    }
}

void MhsWriter::EndStep()
{
    for (auto &e : m_SubEngines)
    {
        e->EndStep();
    }
}

void MhsWriter::Flush(const int transportIndex)
{
    for (auto &e : m_SubEngines)
    {
        e->Flush(transportIndex);
    }
}

// PRIVATE

#define declare_type(T)                                                        \
    void MhsWriter::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PutSyncCommon(variable, data);                                         \
    }                                                                          \
    void MhsWriter::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PutDeferredCommon(variable, data);                                     \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void MhsWriter::DoClose(const int transportIndex)
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
