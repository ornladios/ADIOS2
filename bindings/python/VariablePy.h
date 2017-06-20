/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariablePy.h
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_VARIABLEPY_H_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_VARIABLEPY_H_

#include <adios2.h>

#include "adiosPyFunctions.h"

namespace adios2
{

class VariablePy
{

public:
    const std::string m_Name;
    pyList m_Shape;
    pyList m_Start;
    pyList m_Count;
    const bool m_IsConstantDims;

    VariableBase *m_VariableBase = nullptr;
    bool m_IsDefined = false;

    VariablePy(const std::string &name, const pyList shape, const pyList start,
               const pyList count, const bool isConstantDims,
               const bool debugMode);

    ~VariablePy() = default;

    void SetDimensions(const pyList shape, const pyList start,
                       const pyList count);

    std::string GetType() const noexcept;

private:
    const bool m_DebugMode;
};

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_VARIABLEPY_H_ */
