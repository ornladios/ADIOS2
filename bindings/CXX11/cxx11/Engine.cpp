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

std::string Engine::Name() const
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::Name");
    return m_Engine->m_Name;
}

std::string Engine::Type() const
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::Type");
    return m_Engine->m_EngineType;
}

StepStatus Engine::BeginStep()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::BeginStep");
    return m_Engine->BeginStep();
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    helper::CheckForNullptr(
        m_Engine, "in call to Engine::BeginStep(const StepMode, const float)");
    return m_Engine->BeginStep(mode, timeoutSeconds);
}

size_t Engine::CurrentStep() const
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::CurrentStep");
    return m_Engine->CurrentStep();
}

void Engine::PerformPuts()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::PerformPuts");
    m_Engine->PerformPuts();
}

void Engine::PerformGets()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::PerformGets");
    m_Engine->PerformGets();
}

void Engine::EndStep()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::EndStep");
    m_Engine->EndStep();
}

void Engine::Flush(const int transportIndex)
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::Flush");
    m_Engine->Flush(transportIndex);
}

void Engine::Close(const int transportIndex)
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::Close");
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
    template void Engine::Get<T>(const std::string &, T &, const Mode);        \
                                                                               \
    template void Engine::Get<T>(Variable<T>, std::vector<T> &, const Mode);   \
    template void Engine::Get<T>(const std::string &, std::vector<T> &,        \
                                 const Mode);                                  \
                                                                               \
    template void Engine::Get<T>(                                              \
        Variable<T>, typename Variable<T>::Info & info, const Mode);           \
    template void Engine::Get<T>(                                              \
        const std::string &, typename Variable<T>::Info &info, const Mode);    \
                                                                               \
    template std::map<size_t, std::vector<typename Variable<T>::Info>>         \
    Engine::AllStepsBlocksInfo(const Variable<T> variable) const;              \
                                                                               \
    template std::vector<typename Variable<T>::Info> Engine::BlocksInfo(       \
        const Variable<T> variable, const size_t step) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
