/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOPy.h
 *
 *  Created on: Mar 14, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_IOPY_H_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_IOPY_H_

#include <pybind11/numpy.h>

#include "EnginePy.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

namespace adios2
{

class IOPy
{

public:
    IO &m_IO;

    IOPy(IO &io, const bool debugMode);

    ~IOPy() = default;

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

    EnginePy Open(const std::string &name, const int openMode);

    void FlushAll();

private:
    const bool m_DebugMode;
};

} // end namespace adios2

#endif /* ADIOS2_BINDINGS_PYTHON_SOURCE_IOPY_H_ */
