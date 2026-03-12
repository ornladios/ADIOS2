/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Operator.h"

#include "adios2/core/Operator.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{

Operator::operator bool() const noexcept { return m_Parameters != nullptr; }

std::string Operator::Type() const noexcept { return m_Type; }

void Operator::SetParameter(const std::string key, const std::string value)
{
    helper::CheckForNullptr(m_Parameters, "in call to Operator::SetParameter");
    (*m_Parameters)[key] = value;
}

Params &Operator::Parameters() const
{
    helper::CheckForNullptr(m_Parameters, "in call to Operator::Parameters");
    return *m_Parameters;
}

// PRIVATE
Operator::Operator(const std::string &type, Params *params) : m_Parameters(params), m_Type(type) {}

} // end namespace adios2
