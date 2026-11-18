/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_CORE_DEFINITIONBATCH_H_
#define ADIOS2_CORE_DEFINITIONBATCH_H_

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "adios2/core/Attribute.h"
#include "adios2/core/Variable.h"

namespace adios2
{
namespace core
{

using VarMap = std::unordered_map<std::string, std::unique_ptr<VariableBase>>;
// Attribute order is part of output consistency.
using AttrMap = std::map<std::string, std::unique_ptr<AttributeBase>>;
using VariableOperations =
    std::unordered_map<std::string, std::vector<std::pair<std::string, Params>>>;

class IO;

/** Private definition storage for one worker, created by IO::CreateDefinitionBatch.
 * No engines, transports, ADIOS registration, or access to the destination maps.
 * Different batches may be populated concurrently. Each batch has one owner
 * thread at a time. Names and expressions must use the destination IO namespace;
 * callers loading datasets must qualify references before staging expressions.
 *
 * Associated attributes and derived expressions may precede their inputs or
 * refer to inputs in other batches. They are validated by FinalizeDefinitions,
 * after all batches have been merged. Derived expressions are recorded as text;
 * parsing and binding happen serially during finalization, not on worker threads.
 */
class DefinitionBatch
{
public:
    DefinitionBatch(const DefinitionBatch &) = delete;
    DefinitionBatch &operator=(const DefinitionBatch &) = delete;

    template <class T>
    Variable<T> &DefineVariable(const std::string &name, const Dims &shape = {},
                                const Dims &start = {}, const Dims &count = {},
                                bool constantDims = false);

    template <class T>
    Attribute<T> &
    DefineAttribute(const std::string &name, const T &value, const std::string &variableName = "",
                    const std::string separator = "/", bool allowModification = false);

    template <class T>
    Attribute<T> &DefineAttribute(const std::string &name, const T *values, size_t elements,
                                  const std::string &variableName = "",
                                  const std::string separator = "/",
                                  bool allowModification = false);

    template <class T>
    Variable<T> *InquireVariable(const std::string &name) const noexcept;

    template <class T>
    Attribute<T> *InquireAttribute(const std::string &name, const std::string &variableName = "",
                                   const std::string separator = "/") const noexcept;

#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
    // These stage recipes, rather than returning an incompletely bound variable.
    void DefineDerivedVariable(const std::string &name, const std::string &expression,
                               DerivedVarType type = DerivedVarType::StatsOnly);
    void DefineReaderDerivedVariable(const std::string &name, const std::string &expression);
#endif

    /** True when all objects and pending recipes have been transferred to IO. */
    bool Empty() const noexcept;

private:
    friend class IO;
    DefinitionBatch(const IO &target, ArrayOrdering ordering, const VariableOperations &operations,
                    size_t variableCapacity);
    void CheckVariableName(const std::string &name) const;

    const IO *const m_Target;
    const ArrayOrdering m_ArrayOrder;
    const VariableOperations m_Operations;
    VarMap m_Variables;
    AttrMap m_Attributes;
    std::map<std::string, std::string> m_AttributeVariables;
#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
    struct DerivedDefinition
    {
        std::string Expression;
        DerivedVarType Type;
        bool ReaderSide;
        std::vector<std::pair<std::string, Params>> Operations;
    };
    using DerivedDefinitions = std::map<std::string, DerivedDefinition>;
    DerivedDefinitions m_Derived;
#endif
};

} // namespace core
} // namespace adios2
#endif
