/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.h
 *
 *  Created on: Mar 15, 2017
 *      Author: wgodoy
 */

#ifndef ENGINEPY_H_
#define ENGINEPY_H_

#ifdef HAVE_BOOSTPYTHON
#include "boost/python.hpp"
#include "boost/python/numpy.hpp"
#endif

#ifdef HAVE_PYBIND11
#include "pybind11/numpy.h"
#include "pybind11/pybind11.h"
#endif

#include "ADIOSPy.h"
#include "VariablePy.h"
#include "adiosPyFunctions.h"
#include "core/Engine.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
using dtype = boost::python::numpy::dtype;
#endif

#ifdef HAVE_PYBIND11
using pyArray = pybind11::array;
using dtype = pybind11::dtype;
#endif

class EnginePy
{

public:
    EnginePy(ADIOSPy &adiosPy);

    ~EnginePy();

    std::shared_ptr<Engine> m_Engine;

    void WritePy(VariablePy &variable, const pyArray &array);

    void Advance();

    void Close();

    void GetEngineType() const;

private:
    ADIOSPy &m_ADIOSPy;
    bool m_IsVariableTypeDefined = false;

    template <class T> void DefineVariableInADIOS(VariablePy &variable)
    {
        auto &var = m_ADIOSPy.DefineVariable<T>(
            variable.m_Name, variable.m_LocalDimensions,
            variable.m_GlobalDimensions, variable.m_GlobalOffsets);
        variable.m_VariablePtr = &var;
        variable.m_IsVariableDefined = true;
    }

    template <class T>
    void WriteVariableInADIOS(VariablePy &variable, const pyArray &array)
    {
        m_Engine->Write(
            *reinterpret_cast<Variable<T> *>(variable.m_VariablePtr),
            PyArrayToPointer<T>(array));
    }
};

} // end namespace

#endif /* ENGINEPY_H_ */
