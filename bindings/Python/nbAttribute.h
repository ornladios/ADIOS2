/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ADIOS2_BINDINGS_PYTHON_ATTRIBUTE_H_
#define ADIOS2_BINDINGS_PYTHON_ATTRIBUTE_H_

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include "adios2/core/AttributeBase.h"

namespace nb = nanobind;

namespace adios2
{
namespace py11
{

class IO;

class Attribute
{
    friend class IO;

public:
    Attribute() = default;

    ~Attribute() = default;

    explicit operator bool() const noexcept;

    std::string Name() const;

    std::string Type() const;

    bool SingleValue() const;

    nb::ndarray<nb::numpy> Data();

    std::vector<std::string> DataString();

private:
    Attribute(core::AttributeBase *attribute);
    core::AttributeBase *m_Attribute = nullptr;
};

} // end namespace py11
} // end namespace adios2

#endif /* BINDINGS_PYTHON_PYATTRIBUTE_H_ */
