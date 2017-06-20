/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */

#include "EnginePy.h"

#include "adiosPyFunctions.h"

namespace adios2
{

EnginePy::EnginePy(IO &io, const std::string &name, const OpenMode openMode,
                   MPI_Comm mpiComm)
: m_IO(io), m_Engine(m_IO.Open(name, openMode, mpiComm)),
  m_DebugMode(m_IO.m_DebugMode)
{
}

void EnginePy::Write(VariablePy &variable, const pyArray &array)
{
    if (variable.m_IsDefined)
    {
        // do nothing, not supporting compound types in Python, yet
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<pybind11::array_t<T>>(array))                \
    {                                                                          \
        DefineVariableInIO<T>(variable);                                       \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    if (!variable.m_IsDefined)
    {
        if (m_DebugMode)
        {
            throw std::runtime_error("ERROR: variable " + variable.m_Name +
                                     " couldn't not be created in IO  " +
                                     m_IO.m_Name + " , in call to Write\n");
        }
    }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<pybind11::array_t<T>>(array))                \
    {                                                                          \
        WriteInIO<T>(variable, array);                                         \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
}

void EnginePy::Advance(const float timeoutSeconds)
{
    m_Engine->Advance(timeoutSeconds);
}

void EnginePy::Close(const int transportIndex)
{
    m_Engine->Close(transportIndex);
}

} // end namespace adios
