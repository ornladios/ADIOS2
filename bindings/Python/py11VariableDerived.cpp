/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11Variable.cpp
 */

#include "py11VariableDerived.h"

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

#else

VariableDerived::operator bool() const noexcept { return false; }

std::string VariableDerived::Name() const
{
    return "DerivedVariables are not supported in this ADIOS2 build";
}

DerivedVarType VariableDerived::Type() const { return DerivedVarType::ExpressionString; }

#endif

} // end namespace py11
} // end namespace adios2
