/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_CORE_DEFINITIONHELPERS_H_
#define ADIOS2_CORE_DEFINITIONHELPERS_H_

#include "DefinitionBatch.h"
#include "adios2/helper/adiosFunctions.h"
#include <stdexcept>

namespace adios2
{
namespace core
{
namespace definition
{

// Shared by ordinary IO definitions and private worker batches.
inline void ConfigureOperations(VariableBase &variable, const VariableOperations &operations)
{
    const auto it = operations.find(variable.m_Name);
    if (it != operations.end())
    {
        variable.m_Operations.reserve(it->second.size());
        for (const auto &operation : it->second)
            variable.AddOperation(operation.first, operation.second);
    }
}

inline void ConfigureLayout(VariableBase &variable, ArrayOrdering ordering)
{
#if defined(ADIOS2_HAVE_KOKKOS) || defined(ADIOS2_HAVE_GPU_SUPPORT)
    variable.m_BaseLayout = ordering;
    variable.m_ArrayLayout = ordering;
#else
    (void)variable;
    (void)ordering;
#endif
}

inline void ConfigureVariable(VariableBase &variable, const VariableOperations &operations,
                              ArrayOrdering ordering)
{
    ConfigureOperations(variable, operations);
    ConfigureLayout(variable, ordering);
}

// The bool tells IO whether engines must be notified. Batch definitions never
// notify engines. Both paths retain the same attribute modification semantics.
template <class T, class Create, class Modify>
std::pair<Attribute<T> *, bool> DefineAttribute(AttrMap &attributes, const std::string &name,
                                                const T *values, size_t elements, Create create,
                                                Modify modify)
{
    const auto existing = attributes.find(name);
    if (existing == attributes.end())
    {
        auto attribute = create();
        auto *result = attribute.get();
        attributes.emplace(name, std::move(attribute));
        return {result, true};
    }
    if (existing->second->m_Type != helper::GetDataType<T>())
        helper::Throw<std::invalid_argument>(
            "Core", "Definition", "DefineAttribute",
            "attribute " + name + " with existing type " + ToString(existing->second->m_Type) +
                " cannot be redefined with type " + ToString(helper::GetDataType<T>()));
    auto &attribute = static_cast<Attribute<T> &>(*existing->second);
    if (attribute.Equals(values, elements))
        return {&attribute, false};
    modify(attribute);
    return {&attribute, true};
}

} // namespace definition
} // namespace core
} // namespace adios2
#endif
