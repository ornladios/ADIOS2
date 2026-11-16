/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_BINDINGS_CXX_VARIABLE_DERIVED_H_
#define ADIOS2_BINDINGS_CXX_VARIABLE_DERIVED_H_

#include "Operator.h"
#include "adios2/common/ADIOSTypes.h"

#include <string>
#include <vector>

namespace adios2
{

/// \cond EXCLUDE_FROM_DOXYGEN
// forward declare
class IO; // friend
namespace core
{

class VariableDerived; // private implementation
}
/// \endcond

class VariableDerived
{
    friend class IO;

public:
    /**
     * Empty (default) constructor, use it as a placeholder for future
     * variables from IO:DefineVariableDerived<T> or IO:InquireVariableDerived<T>.
     * Can be used with STL containers.
     */
    VariableDerived() = default;

    /** Default, using RAII STL containers */
    ~VariableDerived() = default;

    /** true: valid object, false: invalid object */
    explicit operator bool() const noexcept;

    /**
     * Inspect derived variable name
     * @return unique name identifier
     */
    std::string Name() const;

    /**
     * Inspect derived variable type
     * @return DerivedVarType set in IO::DefineDerivedVariable
     */
    DerivedVarType Type() const;

    /**
     * Adds operation and parameters to current derived variable object.
     * Only derived variables of type DerivedVarType::StoreData can have
     * operations.
     * @param op operator to be added
     * @param parameters key/value settings particular to the derived variable,
     * not to be confused by op own parameters
     * @return operation index handler in Operations()
     */
    size_t AddOperation(const Operator op, const adios2::Params &parameters = adios2::Params());

    size_t AddOperation(const std::string &type,
                        const adios2::Params &parameters = adios2::Params());

    /**
     * Inspects current operators added with AddOperation
     * @return vector of Operator
     */
    std::vector<Operator> Operations() const;

    /**
     * Removes all current Operations associated with AddOperation.
     */
    void RemoveOperations();

private:
    core::VariableDerived *m_VariableDerived = nullptr;

    VariableDerived(core::VariableDerived *variable);
};

} // end namespace adios2

#endif // ADIOS2_BINDINGS_CXX_VARIABLE_DERIVED_H_
