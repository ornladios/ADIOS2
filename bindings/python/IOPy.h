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

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
/// \endcond

#include "EnginePy.h"
#include "adiosPyTypes.h"

namespace adios
{

class IOPy
{

public:
    IO &m_IO;
    const bool m_DebugMode;

    IOPy(IO &io, const bool debugMode);

    ~IOPy() = default;

    void SetEngine(const std::string engineType);

    void SetParameters(const pyKwargs &kwargs) noexcept;
    unsigned int AddTransport(const std::string type,
                              const pyKwargs &kwargs) noexcept;

    VariablePy &DefineVariable(const std::string &name, const pyList shape,
                               const pyList start, const pyList count,
                               const bool isConstantDims);

    VariablePy &GetVariable(const std::string &name);

    EnginePy Open(const std::string &name, const int openMode);

private:
    /**
     *  Extra map needed as Variables are not created in ADIOS at
     *  DefineVariable, but until Write when type is known from numpy
     */
    std::map<std::string, VariablePy> m_Variables;
};

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_IOPY_H_ */
