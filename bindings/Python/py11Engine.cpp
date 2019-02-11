/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11Engine.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Engine.h"
#include "adios2/helper/adiosFunctions.h"

#include "py11types.h"

namespace adios2
{
namespace py11
{

Engine::Engine(core::Engine *engine) : m_Engine(engine) {}

Engine::operator bool() const noexcept
{
    if (m_Engine == nullptr)
    {
        return false;
    }

    return *m_Engine ? true : false;
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::BeginStep");
    return m_Engine->BeginStep(mode, timeoutSeconds);
}

StepStatus Engine::BeginStep()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::BeginStep");
    return m_Engine->BeginStep();
}

void Engine::Put(Variable variable, const pybind11::array &array,
                 const Mode launch)
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::Put numpy array");
    helper::CheckForNullptr(variable.m_Variable,
                            "for variable, in call to Engine::Put numpy array");

    const std::string type = variable.Type();

    if (type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        m_Engine->Put(*dynamic_cast<core::Variable<T> *>(variable.m_Variable), \
                      reinterpret_cast<const T *>(array.data()), launch);      \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument("ERROR: for variable " + variable.Name() +
                                    " numpy array type is not supported or "
                                    "is not memory contiguous "
                                    ", in call to Put\n");
    }
}

void Engine::Put(Variable variable, const std::string &string)
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::Put string");
    helper::CheckForNullptr(variable.m_Variable,
                            "for variable, in call to Engine::Put string");

    if (variable.Type() != helper::GetType<std::string>())
    {
        throw std::invalid_argument(
            "ERROR: variable " + variable.Name() +
            " is not of string type, in call to Engine::Put");
    }

    m_Engine->Put(
        *dynamic_cast<core::Variable<std::string> *>(variable.m_Variable),
        string);
}

void Engine::PerformPuts()
{
    helper::CheckForNullptr(m_Engine, "in call to PerformPuts");
    m_Engine->PerformPuts();
}

void Engine::Get(Variable variable, pybind11::array &array, const Mode launch)
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::Get a numpy array");

    helper::CheckForNullptr(
        variable.m_Variable,
        "for variable, in call to Engine::Get a numpy array");

    const std::string type = variable.Type();

    if (type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        m_Engine->Get(*dynamic_cast<core::Variable<T> *>(variable.m_Variable), \
                      reinterpret_cast<T *>(const_cast<void *>(array.data())), \
                      launch);                                                 \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        throw std::invalid_argument(
            "ERROR: in variable " + variable.Name() + " of type " +
            variable.Type() +
            ", numpy array type is 1) not supported, 2) a type mismatch or"
            "3) is not memory contiguous "
            ", in call to Get\n");
    }
}

void Engine::Get(Variable variable, std::string &string, const Mode launch)
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::Get a numpy array");

    helper::CheckForNullptr(variable.m_Variable,
                            "for variable, in call to Engine::Get a string");

    const std::string type = variable.Type();

    if (type == helper::GetType<std::string>())
    {
        m_Engine->Get(
            *dynamic_cast<core::Variable<std::string> *>(variable.m_Variable),
            string, launch);
    }
    else
    {
        throw std::invalid_argument("ERROR: variable " + variable.Name() +
                                    " of type " + variable.Type() +
                                    " is not string, in call to Engine::Get");
    }
}

void Engine::PerformGets()
{
    helper::CheckForNullptr(m_Engine, "in call to Engine::PerformGets");
    m_Engine->PerformGets();
}

void Engine::EndStep()
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::EndStep");
    m_Engine->EndStep();
}

void Engine::Flush(const int transportIndex)
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Flush");
    m_Engine->Flush(transportIndex);
}

void Engine::Close(const int transportIndex)
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Close");
    m_Engine->Close(transportIndex);
}

size_t Engine::CurrentStep() const
{
    helper::CheckForNullptr(m_Engine,
                            "for engine, in call to Engine::CurrentStep");
    return m_Engine->CurrentStep();
}

std::string Engine::Name() const
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Name");
    return m_Engine->m_Name;
}

std::string Engine::Type() const
{
    helper::CheckForNullptr(m_Engine, "for engine, in call to Engine::Type");
    return m_Engine->m_EngineType;
}

} // end namespace py11
} // end namespace adios2
