/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "DefinitionBatch.h"
#include "DefinitionHelpers.h"

namespace adios2
{
namespace core
{

DefinitionBatch::DefinitionBatch(const IO &target, ArrayOrdering ordering,
                                 const VariableOperations &operations, size_t variableCapacity)
: m_Target(&target), m_ArrayOrder(ordering), m_Operations(operations)
{
    m_Variables.reserve(variableCapacity);
}

void DefinitionBatch::CheckVariableName(const std::string &name) const
{
    if (m_Variables.count(name)
#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
        || m_Derived.count(name)
#endif
    )
        throw std::invalid_argument("DefinitionBatch: variable already defined: " + name);
}

bool DefinitionBatch::Empty() const noexcept
{
    return m_Variables.empty() && m_Attributes.empty() && m_AttributeVariables.empty()
#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
           && m_Derived.empty()
#endif
        ;
}

template <class T>
Variable<T> &DefinitionBatch::DefineVariable(const std::string &name, const Dims &shape,
                                             const Dims &start, const Dims &count,
                                             bool constantDims)
{
    CheckVariableName(name);
    auto variable = std::make_unique<Variable<T>>(name, shape, start, count, constantDims);
    definition::ConfigureLayout(*variable, m_ArrayOrder);
    auto *result = variable.get();
    m_Variables.emplace(name, std::move(variable));
    return *result;
}

template <class T>
Attribute<T> &DefinitionBatch::DefineAttribute(const std::string &name, const T &value,
                                               const std::string &variableName,
                                               const std::string separator, bool allowModification)
{
    const auto globalName = helper::GlobalName(name, variableName, separator);
    auto result = definition::DefineAttribute<T>(
        m_Attributes, globalName, &value, 1,
        [&] { return std::make_unique<Attribute<T>>(globalName, value, allowModification); },
        [&](Attribute<T> &attribute) { attribute.Modify(value); });
    if (!variableName.empty())
        m_AttributeVariables[globalName] = variableName;
    return *result.first;
}

template <class T>
Attribute<T> &DefinitionBatch::DefineAttribute(const std::string &name, const T *values,
                                               size_t elements, const std::string &variableName,
                                               const std::string separator, bool allowModification)
{
    const auto globalName = helper::GlobalName(name, variableName, separator);
    auto result = definition::DefineAttribute<T>(
        m_Attributes, globalName, values, elements,
        [&] {
            return std::make_unique<Attribute<T>>(globalName, values, elements, allowModification);
        },
        [&](Attribute<T> &attribute) { attribute.Modify(values, elements); });
    if (!variableName.empty())
        m_AttributeVariables[globalName] = variableName;
    return *result.first;
}

template <class T>
Variable<T> *DefinitionBatch::InquireVariable(const std::string &name) const noexcept
{
    const auto it = m_Variables.find(name);
    return it == m_Variables.end() || it->second->m_Type != helper::GetDataType<T>()
               ? nullptr
               : static_cast<Variable<T> *>(it->second.get());
}

template <class T>
Attribute<T> *DefinitionBatch::InquireAttribute(const std::string &name,
                                                const std::string &variableName,
                                                const std::string separator) const noexcept
{
    const auto it = m_Attributes.find(helper::GlobalName(name, variableName, separator));
    return it == m_Attributes.end() || it->second->m_Type != helper::GetDataType<T>()
               ? nullptr
               : static_cast<Attribute<T> *>(it->second.get());
}

#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
void DefinitionBatch::DefineDerivedVariable(const std::string &name, const std::string &expression,
                                            DerivedVarType type)
{
    CheckVariableName(name);
    DerivedDefinition recipe{expression, type, false, {}};
    const auto operations = m_Operations.find(name);
    if (operations != m_Operations.end())
        recipe.Operations = operations->second;
    m_Derived.emplace(name, std::move(recipe));
}

void DefinitionBatch::DefineReaderDerivedVariable(const std::string &name,
                                                  const std::string &expression)
{
    CheckVariableName(name);
    m_Derived.emplace(name, DerivedDefinition{expression, DerivedVarType::StatsOnly, true, {}});
}
#endif

#define instantiate_variable(T)                                                                    \
    template Variable<T> &DefinitionBatch::DefineVariable<T>(const std::string &, const Dims &,    \
                                                             const Dims &, const Dims &, bool);    \
    template Variable<T> *DefinitionBatch::InquireVariable<T>(const std::string &) const noexcept;
ADIOS2_FOREACH_STDTYPE_1ARG(instantiate_variable)
#undef instantiate_variable

#define instantiate_attribute(T)                                                                   \
    template Attribute<T> &DefinitionBatch::DefineAttribute<T>(                                    \
        const std::string &, const T &, const std::string &, const std::string, bool);             \
    template Attribute<T> &DefinitionBatch::DefineAttribute<T>(                                    \
        const std::string &, const T *, size_t, const std::string &, const std::string, bool);     \
    template Attribute<T> *DefinitionBatch::InquireAttribute<T>(                                   \
        const std::string &, const std::string &, const std::string) const noexcept;
ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(instantiate_attribute)
#undef instantiate_attribute

} // namespace core
} // namespace adios2
