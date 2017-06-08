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

    void SetParameters(const pyKwargs &kwargs);
    unsigned int AddTransport(const std::string type, const pyKwargs &kwargs);

    VariablePy DefineVariable(const std::string &name, const pyList shape,
                              const pyList start, const pyList count,
                              const bool isConstantDims);

    EnginePy Open(const std::string &name, const int openMode);

    // EnginePy Open(const std::string &name, const OpenMode openMode);
};

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_IOPY_H_ */
