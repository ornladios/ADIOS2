/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Operator.cpp :
 *
 *  Created on: Jun 7, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Operator.h"

#include "adios2/core/Operator.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

Operator::operator bool() const noexcept
{
    return (m_Operator == nullptr) ? false : true;
}

std::string Operator::Type() const noexcept
{
    if (m_Operator == nullptr)
    {
        return "";
    }

    return m_Operator->m_Type;
}

void Operator::SetParameter(const std::string key, const std::string value)
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::SetParameter");
    m_Operator->SetParameter(key, value);
}

Params Operator::Parameters() const
{
    helper::CheckForNullptr(m_Operator, "in call to Operator::Parameters");
    return m_Operator->GetParameters();
}

// PRIVATE
Operator::Operator(core::Operator *op) : m_Operator(op) {}

std::string ToString(const Operator &op)
{
    return std::string("Operator(Type: \"" + op.Type() + "\")");
}

} // end namespace adios2
