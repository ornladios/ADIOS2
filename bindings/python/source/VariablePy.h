/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariablePy.h
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_PYTHON_SOURCE_VARIABLEPY_H_
#define BINDINGS_PYTHON_SOURCE_VARIABLEPY_H_

#include "adios2/core/VariableBase.h"
#include "adiosPyFunctions.h"

namespace adios
{

class VariablePy
{

public:
    VariablePy(const std::string name, const pyList shape, const pyList start,
               const pyList count, const bool isConstateShape);

    ~VariablePy();

    void SetDimensions(const pyList shape, const pyList start,
                       const pyList count);

    Dims GetLocalDimensions();

    VariableBase *m_VariableBase = nullptr;
    bool m_IsVariableDefined = false;

    const std::string m_Name;
    Dims m_LocalDimensions;
    Dims m_GlobalDimensions;
    Dims m_GlobalOffsets;
};

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_VARIABLEPY_H_ */
