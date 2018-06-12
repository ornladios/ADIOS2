/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.h :
 *
 *  Created on: Jun 4, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Engine.h"
#include "Engine.tcc"

#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

Engine::operator bool() const noexcept
{
    if (m_Engine == nullptr)
    {
        return false;
    }

    return *m_Engine ? true : false;
}

StepStatus Engine::BeginStep()
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::BeginStep");
    return m_Engine->BeginStep();
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    adios2::helper::CheckForNullptr(
        m_Engine, "in call to Engine::BeginStep(const StepMode, const float)");
    return m_Engine->BeginStep(mode, timeoutSeconds);
}

size_t Engine::CurrentStep() const
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::CurrentStep");
    return m_Engine->CurrentStep();
}

void Engine::FixedSchedule()
{
    adios2::helper::CheckForNullptr(m_Engine,
                                    "in call to Engine::FixedSchedule");
    m_Engine->FixedSchedule();
}

void Engine::PerformPuts()
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::PerformPuts");
    m_Engine->PerformPuts();
}

void Engine::PerformGets()
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::PerformGets");
    m_Engine->PerformGets();
}

void Engine::EndStep()
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::EndStep");
    m_Engine->EndStep();
}

void Engine::Flush(const int transportIndex)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Flush");
    m_Engine->Flush(transportIndex);
}

void Engine::Close(const int transportIndex)
{
    adios2::helper::CheckForNullptr(m_Engine, "in call to Engine::Close");
    m_Engine->Close(transportIndex);
}

Engine::Engine(core::Engine *engine) : m_Engine(engine) {}

#define declare_template_instantiation(T)                                      \
    template void Engine::Put<T>(Variable<T>, const T *, const Mode);          \
    template void Engine::Put<T>(const std::string &, const T *, const Mode);  \
    template void Engine::Put<T>(Variable<T>, const T &, const Mode);          \
    template void Engine::Put<T>(const std::string &, const T &, const Mode);  \
                                                                               \
    template void Engine::Get<T>(Variable<T>, T *, const Mode);                \
    template void Engine::Get<T>(const std::string &, T *, const Mode);        \
    template void Engine::Get<T>(Variable<T>, T &, const Mode);                \
    template void Engine::Get<T>(const std::string &, T &, const Mode);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
