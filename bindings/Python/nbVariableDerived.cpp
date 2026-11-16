/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nbVariableDerived.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace py11
{

#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
VariableDerived::VariableDerived(core::VariableDerived *v) : m_VariableDerived(v) {}

VariableDerived::operator bool() const noexcept
{
    return (m_VariableDerived == nullptr) ? false : true;
}

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
    auto params = op.Parameters();
    for (const auto &p : parameters)
    {
        params[p.first] = p.second;
    }
    return AddOperation(op.m_Type, params);
}

size_t VariableDerived::AddOperation(const std::string &op, const Params &parameters)
{
    helper::CheckForNullptr(m_VariableDerived, "in call to VariableDerived::AddOperation");
    if (m_VariableDerived->GetDerivedType() != DerivedVarType::StoreData)
    {
        helper::Throw<std::invalid_argument>("bindings::Python", "VariableDerived", "AddOperation",
                                             "operators can only be applied to derived "
                                             "variables of type DerivedVarType::StoreData");
    }
    return m_VariableDerived->AddOperation(op, parameters);
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

#else

VariableDerived::operator bool() const noexcept { return false; }

std::string VariableDerived::Name() const
{
    return "DerivedVariables are not supported in this ADIOS2 build";
}

DerivedVarType VariableDerived::Type() const { return DerivedVarType::ExpressionString; }

size_t VariableDerived::AddOperation(const Operator, const Params &) { return 0; }

size_t VariableDerived::AddOperation(const std::string &, const Params &) { return 0; }

std::vector<Operator> VariableDerived::Operations() const { return {}; }

void VariableDerived::RemoveOperations() {}

#endif

} // end namespace py11
} // end namespace adios2
