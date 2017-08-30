/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.tcc
 *
 *  Created on: Jun 8, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_ENGINEPY_INL_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_ENGINEPY_INL_
#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_ENGINEPY_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "adiosPyFunctions.h"

namespace adios2
{

template <class T>
void EnginePy::DefineVariableInIO(VariablePy &variable)
{
    auto &var = m_IO.DefineVariable<T>(
        variable.m_Name, PyListToDims(variable.m_Shape),
        PyListToDims(variable.m_Start), PyListToDims(variable.m_Count),
        variable.m_IsConstantDims);

    variable.m_VariableBase = &var;
    variable.m_IsDefined = true;
}

template <class T>
void EnginePy::WriteInIO(VariablePy &variable, const pyArray &array)
{
    m_Engine.Write(*dynamic_cast<Variable<T> *>(variable.m_VariableBase),
                   reinterpret_cast<const T *>(array.data()));
}

} // end namespace adios2

#endif /* BINDINGS_PYTHON_SOURCE_ENGINEPY_INL_ */
