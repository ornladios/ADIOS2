/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.h
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */

#ifndef BINDINGS_PYTHON_SOURCE_ENGINEPY_H_
#define BINDINGS_PYTHON_SOURCE_ENGINEPY_H_

#include "ADIOSPy.h"
#include "VariablePy.h"
#include "adios2/core/Engine.h"
#include "adiosPyFunctions.h"

namespace adios
{
class EnginePy
{

public:
    EnginePy();

    ~EnginePy();

    Engine *m_Engine;

    void WritePy(VariablePy &variable, const pyArray &array);

    void Advance();

    void Close();

private:
    template <class T>
    void DefineVariableInADIOS(VariablePy &variable);

    template <class T>
    void WriteVariableInADIOS(VariablePy &variable, const pyArray &array);

    //    template <class T>
    //    void DefineVariableInADIOS(VariablePy &variable)
    //    {
    //        auto &var = m_ADIOSPy.DefineVariable<T>(
    //            variable.m_Name, variable.m_LocalDimensions,
    //            variable.m_GlobalDimensions, variable.m_GlobalOffsets);
    //        variable.m_VariablePtr = &var;
    //        variable.m_IsVariableDefined = true;
    //    }
    //
    //    template <class T>
    //    void WriteVariableInADIOS(VariablePy &variable, const pyArray &array)
    //    {
    //        m_Engine->Write(
    //            *reinterpret_cast<Variable<T> *>(variable.m_VariablePtr),
    //            PyArrayToPointer<T>(array));
    //    }
};

} // end namespace

#endif /* BINDINGS_PYTHON_SOURCE_ENGINEPY_H_ */
