/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "VariableDerived.h"

#include "adios2/core/VariableDerived.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
VariableDerived::VariableDerived(core::VariableDerived *variable) : m_VariableDerived(variable) {}

VariableDerived::operator bool() const noexcept { return (m_VariableDerived != nullptr); }

std::string VariableDerived::Name() const
{
    helper::CheckForNullptr(m_VariableDerived, "in call to VariableDerived::Name");
    return m_VariableDerived->m_Name;
}

DerivedVarType VariableDerived::Type() const
{
    helper::CheckForNullptr(m_VariableDerived, "in call to VariableDerived::Type");
    return m_VariableDerived->GetDerivedType();
}

size_t VariableDerived::AddOperation(const Operator op, const Params &parameters)
{
    if (!op)
    {
        throw std::invalid_argument("ERROR: invalid operator, in call to "
                                    "VariableDerived::AddOperation");
    }
    auto params = op.Parameters();
    for (const auto &p : parameters)
    {
        params[p.first] = p.second;
    }
    return AddOperation(op.m_Type, params);
}

size_t VariableDerived::AddOperation(const std::string &type, const Params &parameters)
{
    helper::CheckForNullptr(m_VariableDerived, "in call to VariableDerived::AddOperation");
    if (m_VariableDerived->GetDerivedType() != DerivedVarType::StoreData)
    {
        helper::Throw<std::invalid_argument>("bindings::CXX11", "VariableDerived", "AddOperation",
                                             "operators can only be applied to derived "
                                             "variables of type DerivedVarType::StoreData");
    }
    return m_VariableDerived->AddOperation(type, parameters);
}

std::vector<Operator> VariableDerived::Operations() const
{
    helper::CheckForNullptr(m_VariableDerived, "in call to VariableDerived::Operations");
    std::vector<Operator> operations;
    operations.reserve(m_VariableDerived->m_Operations.size());
    for (const auto &op : m_VariableDerived->m_Operations)
    {
        operations.push_back(Operator(op->m_TypeString, &op->GetParameters()));
    }
    return operations;
}

void VariableDerived::RemoveOperations()
{
    helper::CheckForNullptr(m_VariableDerived, "in call to VariableDerived::RemoveOperations");
    m_VariableDerived->RemoveOperations();
}
} // end namespace adios2
