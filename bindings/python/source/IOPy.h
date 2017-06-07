/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOPy.h
 *
 *  Created on: Mar 14, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef BINDINGS_PYTHON_SOURCE_IOPY_H_
#define BINDINGS_PYTHON_SOURCE_IOPY_H_

#include "adios2/core/IO.h"
#include "adiosPyTypes.h"

namespace adios
{

class IOPy
{

public:
    IOPy(IO &io, const bool debugMode);

    ~IOPy() = default;

    void SetParameters(const pyKwargs kwargs);
    unsigned int AddTransport(const std::string type, const pyKwargs kwargs);

    VariablePy DefineVariable(const std::string &name, const pyList shape,
                              const pyList start, const pyList count,
                              const bool isConstantShape = false);

    EnginePy Open(const std::string name, const std::string openMode,
                  pyObject py_comm = pyObject());

private:
    IO &m_IO;
    const bool m_DebugMode;
};

} // end namespace adios

#endif /* BINDINGS_PYTHON_SOURCE_IOPY_H_ */
