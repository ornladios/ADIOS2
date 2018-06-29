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

Engine::Engine(core::IO &io, const std::string &name, const Mode openMode,
               MPI_Comm mpiComm)
: m_Engine(io.Open(name, openMode, mpiComm)), m_DebugMode(io.m_DebugMode)
{
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    return m_Engine.BeginStep(mode, timeoutSeconds);
}

void Engine::Put(core::VariableBase *variable, const pybind11::array &array,
                 const Mode launch)
{
    helper::CheckForNullptr(variable,
                            "for variable, in call to Put numpy array");

    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == helper::GetType<T>())                         \
    {                                                                          \
        m_Engine.Put(*dynamic_cast<core::Variable<T> *>(variable),             \
                     reinterpret_cast<const T *>(array.data()), launch);       \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: variable " + variable->m_Name +
                                        " numpy array type is not supported or "
                                        "is not memory contiguous "
                                        ", in call to Put\n");
        }
    }
}

void Engine::Put(core::VariableBase *variable, const std::string &string)
{
    helper::CheckForNullptr(variable,
                            "for variable, in call to PutSync string");

    m_Engine.Put(*dynamic_cast<core::Variable<std::string> *>(variable),
                 string);
}

void Engine::PerformPuts() { m_Engine.PerformPuts(); }

void Engine::Get(core::VariableBase *variable, pybind11::array &array,
                 const Mode launch)
{
    helper::CheckForNullptr(variable,
                            "for variable, in call to GetSync a numpy array");

    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == helper::GetType<T>())                         \
    {                                                                          \
        m_Engine.Get(*dynamic_cast<core::Variable<T> *>(variable),             \
                     reinterpret_cast<T *>(const_cast<void *>(array.data())),  \
                     launch);                                                  \
    }
    ADIOS2_FOREACH_NUMPY_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: in variable " + variable->m_Name + " of type " +
                variable->m_Type +
                ", numpy array type is 1) not supported, 2) a type mismatch or"
                "3) is not memory contiguous "
                ", in call to Get\n");
        }
    }
}

void Engine::Get(core::VariableBase *variable, std::string &string,
                 const Mode launch)
{
    helper::CheckForNullptr(variable,
                            "for variable, in call to GetSync a string");

    if (variable->m_Type == helper::GetType<std::string>())
    {
        m_Engine.Get(*dynamic_cast<core::Variable<std::string> *>(variable),
                     string, launch);
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: variable " + variable->m_Name +
                                        " of type " + variable->m_Type +
                                        " is not string, in call to Get");
        }
    }
}

void Engine::PerformGets() { m_Engine.PerformGets(); }

void Engine::EndStep() { m_Engine.EndStep(); }

void Engine::Flush(const int transportIndex) { m_Engine.Flush(transportIndex); }

void Engine::Close(const int transportIndex) { m_Engine.Close(transportIndex); }

size_t Engine::CurrentStep() const { return m_Engine.CurrentStep(); }

std::string Engine::Name() const noexcept { return m_Engine.m_Name; }
std::string Engine::Type() const noexcept { return m_Engine.m_EngineType; }

} // end namespace py11
} // end namespace adios2
