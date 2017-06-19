/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * VariablePy.cpp
 *
 *  Created on: Mar 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "VariablePy.h"

namespace adios2
{

VariablePy::VariablePy(const std::string &name, const pyList shape,
                       const pyList start, const pyList count,
                       const bool isConstantDims, const bool debugMode)
: m_Name(name), m_Shape(shape), m_Start(start), m_Count(count),
  m_IsConstantDims(isConstantDims), m_DebugMode(debugMode)
{
}

void VariablePy::SetDimensions(const pyList shape, const pyList start,
                               const pyList count)
{
    if (m_DebugMode)
    {
        if (m_IsConstantDims)
        {
            throw std::invalid_argument(
                "ERROR: variable " + m_Name +
                " dimensions are constant, in call from SetDimensions\n");
        }
    }

    m_Shape = shape;
    m_Start = start;
    m_Count = count;
}

std::string VariablePy::GetType() const noexcept
{
    return m_VariableBase->m_Type;
}

} // end namespace adios
