/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariablePy.h
 *
 *  Created on: Mar 13, 2017
 *      Author: wgodoy
 */

#ifndef VARIABLEPY_H_
#define VARIABLEPY_H_

#include "adiosPyFunctions.h"
#include "core/Variable.h"

namespace adios
{

#ifdef HAVE_BOOSTPYTHON
using pyList = boost::python::list;
#endif

#ifdef HAVE_PYBIND11
using pyList = pybind11::list;
#endif

class VariablePy
{

public:
    VariablePy(const std::string name, const pyList localDimensionsPy,
               const pyList globalDimensionsPy, const pyList globalOffsetsPy);

    ~VariablePy();

    void SetLocalDimensions(const pyList list);

    void SetGlobalDimensionsAndOffsets(const pyList globalDimensions,
                                       const pyList globalOffsets);

    Dims GetLocalDimensions();

    void *m_VariablePtr = nullptr;
    bool m_IsVariableDefined = false;

    const std::string m_Name;
    Dims m_LocalDimensions;
    Dims m_GlobalDimensions;
    Dims m_GlobalOffsets;
};

} // end namespace

#endif /* BINDINGS_PYTHON_INCLUDE_VARIABLEPY_H_ */
