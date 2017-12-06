/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "EnginePy.h"
#include "typesPy.h"

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

EnginePy::EnginePy(IO &io, const std::string &name, const Mode openMode,
                   MPI_Comm mpiComm)
: m_Engine(io.Open(name, openMode, mpiComm)), m_DebugMode(io.m_DebugMode)
{
}

void EnginePy::BeginStep() { m_Engine.BeginStep(); }

void EnginePy::PutSync(VariableBase *variable, const pybind11::array &array)
{
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

void EnginePy::PutSync(VariableBase *variable, const std::string &string)
{
    m_Engine.PutSync(*dynamic_cast<adios2::Variable<std::string> *>(variable),
                     string);
}

void EnginePy::PutDeferred(VariableBase *variable, const pybind11::array &array)
{
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

void EnginePy::PutDeferred(VariableBase *variable, const std::string &string)
{
    m_Engine.PutDeferred(
        *dynamic_cast<adios2::Variable<std::string> *>(variable), string);
}

void EnginePy::PerformPuts() { m_Engine.PerformPuts(); }

void EnginePy::GetSync(VariableBase *variable, pybind11::array &array)
{
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

void EnginePy::GetSync(VariableBase *variable, std::string &string)
{
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

void EnginePy::GetDeferred(VariableBase *variable, pybind11::array &array)
{
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

void EnginePy::GetDeferred(VariableBase *variable, std::string &string)
{
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
void EnginePy::PerformGets() { m_Engine.PerformGets(); }

void EnginePy::EndStep() { m_Engine.EndStep(); }

void EnginePy::WriteStep() { m_Engine.WriteStep(); }

void EnginePy::Close(const int transportIndex)
{
    m_Engine.Close(transportIndex);
}

} // end namespace adios2
