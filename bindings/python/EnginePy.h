/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.h
 *
 *  Created on: Mar 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_SOURCE_ENGINEPY_H_
#define ADIOS2_BINDINGS_PYTHON_SOURCE_ENGINEPY_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <memory> //std::shared_ptr
#include <string>
/// \endcond

#include <adios2.h>

#include "VariablePy.h"
#include "adiosPyTypes.h" //pyArray

namespace adios2
{

class EnginePy
{

public:
    EnginePy(IO &io, const std::string &name, const Mode openMode,
             MPI_Comm mpiComm);

    ~EnginePy() = default;

    void Write(VariablePy &variable, const pyArray &array);

    void Advance(const float timeoutSeconds = 0.);

    void Close(const int transportIndex = -1);

private:
    IO &m_IO;
    Engine &m_Engine;
    const bool m_DebugMode;

    template <class T>
    void DefineVariableInIO(VariablePy &variable);

    template <class T>
    void WriteInIO(VariablePy &variable, const pyArray &array);
};

} // end namespace adios

#include "EnginePy.inl"

#endif /* BINDINGS_PYTHON_SOURCE_ENGINEPY_H_ */
