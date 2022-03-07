/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * SscWriter.cpp
 *
 *  Created on: Nov 1, 2018
 *      Author: Jason Wang
 */

#include "SscWriter.h"
#include "SscWriterGeneric.h"
#include "adios2/helper/adiosCommMPI.h"
#include "adios2/helper/adiosString.h"
#include <adios2-perfstubs-interface.h>

namespace adios2
{
namespace core
{
namespace engine
{

SscWriter::SscWriter(IO &io, const std::string &name, const Mode mode,
                     helper::Comm comm)
: Engine("SscWriter", io, name, mode, std::move(comm))
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    helper::GetParameter(m_IO.m_Parameters, "EngineMode", m_EngineMode);
    helper::GetParameter(m_IO.m_Parameters, "Verbose", m_Verbosity);

    if (m_EngineMode == "generic")
    {
        m_EngineInstance = std::make_shared<ssc::SscWriterGeneric>(
            io, name, mode, CommAsMPI(m_Comm));
    }
    else if (m_EngineMode == "naive")
    {
    }
}

StepStatus SscWriter::BeginStep(StepMode mode, const float timeoutSeconds)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    helper::Log("Engine", "SSCWriter", "BeginStep",
                std::to_string(CurrentStep()), 0, m_Comm.Rank(), 5, m_Verbosity,
                helper::LogMode::INFO);

    return m_EngineInstance->BeginStep(mode, timeoutSeconds,
                                       m_WriterDefinitionsLocked);
}

size_t SscWriter::CurrentStep() const
{
    return m_EngineInstance->CurrentStep();
}

void SscWriter::PerformPuts()
{
    PERFSTUBS_SCOPED_TIMER_FUNC();
    m_EngineInstance->PerformPuts();
}

void SscWriter::EndStep()
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    helper::Log("Engine", "SSCWriter", "EndStep", std::to_string(CurrentStep()),
                0, m_Comm.Rank(), 5, m_Verbosity, helper::LogMode::INFO);

    m_EngineInstance->EndStep(m_WriterDefinitionsLocked);
}

void SscWriter::DoClose(const int transportIndex)
{
    PERFSTUBS_SCOPED_TIMER_FUNC();

    helper::Log("Engine", "SSCWriter", "Close", m_Name, 0, m_Comm.Rank(), 5,
                m_Verbosity, helper::LogMode::INFO);

    m_EngineInstance->Close(transportIndex);
}

#define declare_type(T)                                                        \
    void SscWriter::DoPutSync(Variable<T> &variable, const T *data)            \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER_FUNC();                                         \
        helper::Log("Engine", "SSCWriter", "PutSync", variable.m_Name, 0,      \
                    m_Comm.Rank(), 5, m_Verbosity, helper::LogMode::INFO);     \
        m_EngineInstance->PutDeferred(variable, data);                         \
        m_EngineInstance->PerformPuts();                                       \
    }                                                                          \
    void SscWriter::DoPutDeferred(Variable<T> &variable, const T *data)        \
    {                                                                          \
        PERFSTUBS_SCOPED_TIMER_FUNC();                                         \
        helper::Log("Engine", "SSCWriter", "PutDeferred", variable.m_Name, 0,  \
                    m_Comm.Rank(), 5, m_Verbosity, helper::LogMode::INFO);     \
        m_EngineInstance->PutDeferred(variable, data);                         \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

void SscWriter::Flush(const int transportIndex) {}

} // end namespace engine
} // end namespace core
} // end namespace adios2
