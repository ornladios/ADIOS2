/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11VariableDerived.h
 *
 */

#ifndef ADIOS2_BINDINGS_PYTHON_VARIABLEDERIVED_H_
#define ADIOS2_BINDINGS_PYTHON_VARIABLEDERIVED_H_

#include "adios2/common/ADIOSConfig.h"

#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
#include "adios2/core/VariableDerived.h"
#else
#include "adios2/common/ADIOSTypes.h"
#endif

namespace adios2
{
namespace py11
{

class IO;
class Engine;

class VariableDerived
{
    friend class IO;
    friend class Engine;

public:
    VariableDerived() = default;

    ~VariableDerived() = default;

    explicit operator bool() const noexcept;

    std::string Name() const;

    DerivedVarType Type() const;

private:
#ifdef ADIOS2_HAVE_DERIVED_VARIABLE
    VariableDerived(adios2::core::VariableDerived *v);
    adios2::core::VariableDerived *m_VariableDerived = nullptr;
#endif
};

} // end namespace py11
} // end namespace adios2

#endif /* ADIOS2_BINDINGS_PYTHON_VARIABLEDERIVED_H_ */
