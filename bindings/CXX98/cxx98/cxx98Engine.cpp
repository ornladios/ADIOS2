/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * cxx98Engine.cpp
 *
 *  Created on: Apr 10, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "cxx98Engine.h"
#include "cxx98Engine.tcc"

namespace adios2
{
namespace cxx98
{

namespace
{

StepStatus ToStepStatus(const adios2_step_status status)
{
    if (status == adios2_step_status_ok)
    {
        return StepStatus::OK;
    }
    else if (status == adios2_step_status_not_ready)
    {
        return StepStatus::NotReady;
    }
    else if (status == adios2_step_status_end_of_stream)
    {
        return StepStatus::EndOfStream;
    }

    return StepStatus::OtherError;
}

adios2_step_mode FromStepMode(const StepMode mode)
{
    if (mode == AtEnd)
    {
        return adios2_step_mode_append;
    }
    else if (mode == Update)
    {
        return adios2_step_mode_update;
    }
    else if (mode == NextAvailable)
    {
        return adios2_step_mode_next_available;
    }
    else if (mode == LatestAvailable)
    {
        return adios2_step_mode_latest_available;
    }

    return adios2_step_mode_update;
}
} // end empty namespace

Engine::Engine(adios2_engine &engine) : m_Engine(engine) {}

Engine::~Engine() {}

StepStatus Engine::BeginStep()
{
    const adios2_step_status status =
        adios2_begin_step(&m_Engine, adios2_step_mode_next_available, 0.f);
    return ToStepStatus(status);
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    const adios2_step_status status =
        adios2_begin_step(&m_Engine, FromStepMode(mode), timeoutSeconds);
    return ToStepStatus(status);
}

size_t Engine::CurrentStep() const { return adios2_current_step(&m_Engine); }

void Engine::EndStep() { adios2_end_step(&m_Engine); }

void Engine::PerformPuts() { adios2_perform_puts(&m_Engine); }

void Engine::PerformGets() { adios2_perform_gets(&m_Engine); }

void Engine::WriteStep() { adios2_write_step(&m_Engine); }

void Engine::Flush(const int transportIndex)
{
    adios2_flush_by_index(&m_Engine, transportIndex);
}

void Engine::Close(const int transportIndex)
{
    adios2_close_by_index(&m_Engine, transportIndex);
}

#define declare_template_instantiation(T)                                      \
    template void Engine::PutSync<T>(Variable<T> &, const T *);                \
                                                                               \
    template void Engine::PutDeferred<T>(Variable<T> &, const T *);            \
                                                                               \
    template void Engine::GetSync<T>(Variable<T> &, T *);                      \
                                                                               \
    template void Engine::GetDeferred<T>(Variable<T> &, T *);

ADIOS2_FOREACH_CXX98_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace cxx98
} // end namespace adios2
