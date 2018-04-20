/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Engine.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

#include "py11Engine.h"
#include "py11types.h"

namespace adios2
{
namespace py11
{

Engine::Engine(adios2::IO &io, const std::string &name, const Mode openMode,
               MPI_Comm mpiComm)
: m_Engine(io.Open(name, openMode, mpiComm)), m_DebugMode(io.m_DebugMode)
{
}

StepStatus Engine::BeginStep(const StepMode mode, const float timeoutSeconds)
{
    return m_Engine.BeginStep(mode, timeoutSeconds);
}

void Engine::PutSync(VariableBase *variable, const pybind11::array &array)
{
    adios2::CheckForNullptr(variable,
                            "for variable, in call to PutSync a numpy array");

    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == GetType<T>())                                 \
    {                                                                          \
        m_Engine.PutSync(*dynamic_cast<adios2::Variable<T> *>(variable),       \
                         reinterpret_cast<const T *>(array.data()));           \
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
                                        ", in call to PutSync\n");
        }
    }
}

void Engine::PutSync(VariableBase *variable, const std::string &string)
{
    adios2::CheckForNullptr(variable,
                            "for variable, in call to PutSync string");

    m_Engine.PutSync(*dynamic_cast<adios2::Variable<std::string> *>(variable),
                     string);
}

void Engine::PutDeferred(VariableBase *variable, const pybind11::array &array)
{
    adios2::CheckForNullptr(
        variable, "for variable, in call to PutDeferred a numpy array");

    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == GetType<T>())                                 \
    {                                                                          \
        m_Engine.PutDeferred(*dynamic_cast<adios2::Variable<T> *>(variable),   \
                             reinterpret_cast<const T *>(array.data()));       \
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
                                        ", in call to PutDeferred\n");
        }
    }
}

void Engine::PutDeferred(VariableBase *variable, const std::string &string)
{
    adios2::CheckForNullptr(variable,
                            "for variable, in call to PutDeferred a string");
    m_Engine.PutDeferred(
        *dynamic_cast<adios2::Variable<std::string> *>(variable), string);
}

void Engine::PerformPuts() { m_Engine.PerformPuts(); }

void Engine::GetSync(VariableBase *variable, pybind11::array &array)
{
    adios2::CheckForNullptr(variable,
                            "for variable, in call to GetSync a numpy array");

    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == GetType<T>())                                 \
    {                                                                          \
        m_Engine.GetSync(                                                      \
            *dynamic_cast<adios2::Variable<T> *>(variable),                    \
            reinterpret_cast<T *>(const_cast<void *>(array.data())));          \
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
                ", in call to GetSync\n");
        }
    }
}

void Engine::GetSync(VariableBase *variable, std::string &string)
{
    adios2::CheckForNullptr(variable,
                            "for variable, in call to GetSync a string");

    if (variable->m_Type == GetType<std::string>())
    {
        m_Engine.GetSync(
            *dynamic_cast<adios2::Variable<std::string> *>(variable), string);
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: variable " + variable->m_Name +
                                        " of type " + variable->m_Type +
                                        " is not string, in call to GetSync");
        }
    }
}

void Engine::GetDeferred(VariableBase *variable, pybind11::array &array)
{
    adios2::CheckForNullptr(
        variable, "for variable, in call to GetDeferred a numpy array");

    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == GetType<T>())                                 \
    {                                                                          \
        m_Engine.GetDeferred(                                                  \
            *dynamic_cast<adios2::Variable<T> *>(variable),                    \
            reinterpret_cast<T *>(const_cast<void *>(array.data())));          \
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
                ", in call to GetSync\n");
        }
    }
}

void Engine::GetDeferred(VariableBase *variable, std::string &string)
{
    adios2::CheckForNullptr(variable,
                            "for variable, in call to GetDeferred a string");

    if (variable->m_Type == GetType<std::string>())
    {
        m_Engine.GetDeferred(
            *dynamic_cast<adios2::Variable<std::string> *>(variable), string);
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument(
                "ERROR: variable " + variable->m_Name + " of type " +
                variable->m_Type + " is not string, in call to GetDeferred");
        }
    }
}
void Engine::PerformGets() { m_Engine.PerformGets(); }

void Engine::EndStep() { m_Engine.EndStep(); }

void Engine::WriteStep() { m_Engine.WriteStep(); }

void Engine::Flush(const int transportIndex) { m_Engine.Flush(transportIndex); }

void Engine::Close(const int transportIndex) { m_Engine.Close(transportIndex); }

size_t Engine::CurrentStep() const { return m_Engine.CurrentStep(); }

} // end namespace py11
} // end namespace adios2
