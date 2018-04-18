/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11IO.h
 *
 *  Created on: Mar 14, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_PY11IO_H_
#define ADIOS2_BINDINGS_PYTHON_PY11IO_H_

#include <pybind11/numpy.h>

#include <string>

#include "py11Engine.h"

namespace adios2
{
namespace py11
{

class IO
{

public:
    adios2::IO &m_IO;

    IO(adios2::IO &io, const bool debugMode);

    ~IO() = default;

    void SetEngine(const std::string type) noexcept;
    unsigned int AddTransport(const std::string type, const Params &parameters);
    void SetParameters(const Params &parameters) noexcept;
    void SetParameter(const std::string key, const std::string value) noexcept;

    const Params &GetParameters() const noexcept;

    VariableBase *DefineVariable(const std::string &name,
                                 std::string &stringValue);

    VariableBase *DefineVariable(const std::string &name, const Dims &shape,
                                 const Dims &start, const Dims &count,
                                 const bool isConstantDims,
                                 pybind11::array &array);

    VariableBase *InquireVariable(const std::string &name) noexcept;

    AttributeBase *DefineAttribute(const std::string &name,
                                   pybind11::array &array);

    AttributeBase *DefineAttribute(const std::string &name,
                                   const std::vector<std::string> &strings);

    Engine Open(const std::string &name, const int openMode);

    void FlushAll();

private:
    const bool m_DebugMode;
};

} // end namespace py11
} // end namespace adios2

#endif
