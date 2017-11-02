/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.cpp
 *
 *  Created on: Dec 19, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Engine.h"
#include "Engine.tcc"

#include <stdexcept>

namespace adios2
{

Engine::Engine(const std::string engineType, IO &io, const std::string &name,
               const Mode openMode, MPI_Comm mpiComm)
: m_EngineType(engineType), m_IO(io), m_Name(name), m_OpenMode(openMode),
  m_MPIComm(mpiComm), m_DebugMode(io.m_DebugMode)
{
}

IO &Engine::GetIO() noexcept { return m_IO; }

StepStatus Engine::BeginStep()
{
    if (m_OpenMode == Mode::Read)
    {
        return BeginStep(StepMode::NextAvailable, 0.0f);
    }
    else
    {
        return BeginStep(StepMode::Append, 0.0f);
    }
}

StepStatus Engine::BeginStep(StepMode mode, const float timeoutSeconds)
{
    ThrowUp("BeginStep");
    return StepStatus::OtherError;
}
void Engine::EndStep() { ThrowUp("EndStep"); }

void Engine::PutSync(const std::string &variableName)
{
    const std::string type(m_IO.InquireVariableType(variableName));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(variableName);         \
        if (m_DebugMode && variable == nullptr)                                \
        {                                                                      \
            throw std::invalid_argument("ERROR: variable " + variableName +    \
                                        " not found, in call to PutSync\n");   \
        }                                                                      \
        PutSync<T>(*variable);                                                 \
    }

    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void Engine::PutDeferred(const std::string &variableName)
{
    const std::string type(m_IO.InquireVariableType(variableName));

    if (type == "compound")
    {
        // not supported
    }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(variableName);         \
        if (m_DebugMode && variable == nullptr)                                \
        {                                                                      \
            throw std::invalid_argument("ERROR: variable " + variableName +    \
                                        " not found, in call to PutSync\n");   \
        }                                                                      \
        PutDeferred<T>(*variable);                                             \
    }

    ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
}

void Engine::PerformPuts() { ThrowUp("PerformPuts"); }
void Engine::PerformGets() { ThrowUp("PerformGets"); }

void Engine::WriteStep()
{
    BeginStep();

    const auto &variablesDataMap = m_IO.GetVariablesDataMap();
    for (const auto &variablePair : variablesDataMap)
    {
        const std::string variableName(variablePair.first);
        const std::string type(variablePair.second.first);

        if (type == "compound")
        {
            // not supported
        }
#define declare_template_instantiation(T)                                      \
    else if (type == adios2::GetType<T>())                                     \
    {                                                                          \
        Variable<T> *variable = m_IO.InquireVariable<T>(variableName);         \
        if (variable->GetData() != nullptr)                                    \
        {                                                                      \
            PutDeferred<T>(*variable, variable->GetData());                    \
        }                                                                      \
    }

        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
    PerformPuts();
    EndStep();
}
void Engine::ReadStep() { ThrowUp("ReadStep"); }

// PROTECTED
void Engine::Init() {}
void Engine::InitParameters() {}
void Engine::InitTransports() {}

// Put
#define declare_type(T)                                                        \
    void Engine::DoPutSync(Variable<T> &, const T *) { ThrowUp("DoPutSync"); } \
    void Engine::DoPutDeferred(Variable<T> &, const T *)                       \
    {                                                                          \
        ThrowUp("DoPutDeferred");                                              \
    }                                                                          \
    void Engine::DoPutDeferred(Variable<T> &, const T &)                       \
    {                                                                          \
        ThrowUp("DoPutDeferred");                                              \
    };
ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

// Get
#define declare_type(T)                                                        \
    void Engine::DoGetSync(Variable<T> &, T *) { ThrowUp("DoGetSync"); }       \
    void Engine::DoGetDeferred(Variable<T> &, T *)                             \
    {                                                                          \
        ThrowUp("DoGetDeferred");                                              \
    }                                                                          \
    void Engine::DoGetDeferred(Variable<T> &, T &) { ThrowUp("DoGetDeferred"); }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

// PRIVATE
void Engine::ThrowUp(const std::string function) const
{
    throw std::invalid_argument("ERROR: Engine derived class " + m_EngineType +
                                " doesn't implement function " + function +
                                "\n");
}

// PUBLIC TEMPLATE FUNCTIONS EXPANSION WITH SCOPED TYPES
#define declare_template_instantiation(T)                                      \
    template void Engine::PutSync<T>(Variable<T> &);                           \
    template void Engine::PutDeferred<T>(Variable<T> &);                       \
                                                                               \
    template void Engine::PutSync<T>(Variable<T> &, const T *);                \
    template void Engine::PutDeferred<T>(Variable<T> &, const T *);            \
    template void Engine::PutSync<T>(const std::string &, const T *);          \
    template void Engine::PutDeferred<T>(const std::string &, const T *);      \
                                                                               \
    template void Engine::PutSync<T>(Variable<T> &, const T &);                \
    template void Engine::PutDeferred<T>(Variable<T> &, const T &);            \
    template void Engine::PutSync<T>(const std::string &, const T &);          \
    template void Engine::PutDeferred<T>(const std::string &, const T &);      \
                                                                               \
    template void Engine::GetSync<T>(Variable<T> &);                           \
    template void Engine::GetDeferred<T>(Variable<T> &);                       \
                                                                               \
    template void Engine::GetSync<T>(Variable<T> &, T *);                      \
    template void Engine::GetDeferred<T>(Variable<T> &, T *);                  \
    template void Engine::GetSync<T>(const std::string &, T *);                \
    template void Engine::GetDeferred<T>(const std::string &, T *);            \
                                                                               \
    template void Engine::GetSync<T>(Variable<T> &, T &);                      \
    template void Engine::GetDeferred<T>(Variable<T> &, T &);                  \
    template void Engine::GetSync<T>(const std::string &, T &);                \
    template void Engine::GetDeferred<T>(const std::string &, T &);

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios2
