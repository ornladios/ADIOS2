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

#include "adios2/ADIOSMacros.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

EnginePy::EnginePy(IO &io, const std::string &name, const Mode openMode,
                   MPI_Comm mpiComm,
                   std::map<std::string, VariableBase> &variablesPlaceholder)
: m_Engine(io.Open(name, openMode, mpiComm)),
  m_VariablesPlaceholder(variablesPlaceholder), m_DebugMode(io.m_DebugMode)
{
}

void EnginePy::PutSync(VariableBase *variable, const pybind11::array &array)
{
    if (variable->m_Type.empty()) // Define in IO
    {
        auto &io = m_Engine.GetIO();

        if (array == pybind11::array())
        {
            if (m_DebugMode)
            {
                throw std::invalid_argument(
                    "ERROR: passing an empty numpy array for variable " +
                    variable->m_Name + ", in call to PutSync");
            }
        }
#define declare_type(T)                                                        \
    else if (pybind11::isinstance<                                             \
                 pybind11::array_t<T, pybind11::array::c_style>>(array))       \
    {                                                                          \
        variable = &io.DefineVariable<T>(variable->m_Name, variable->m_Shape,  \
                                         variable->m_Start, variable->m_Count, \
                                         variable->m_ConstantDims);            \
        m_VariablesPlaceholder.erase(variable->m_Name);                        \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    // PutSync
    if (variable->m_Type == "compound")
    {
        // not supported
    }
#define declare_type(T)                                                        \
    else if (variable->m_Type == GetType<T>())                                 \
    {                                                                          \
        m_Engine.PutSync(dynamic_cast<adios2::Variable<T> &>(variable),        \
                         reinterpret_cast<const T *>(array.data()));           \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: variable " + variable.m_Name +
                                        " numpy array type is not supported or "
                                        "is not memory contiguous "
                                        ", in call to PutSync\n");
        }
    }
}

void EnginePy::EndStep() { m_Engine.EndStep(); }

void EnginePy::Close(const int transportIndex)
{
    m_Engine.Close(transportIndex);
}

} // end namespace adios2
