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
#include <pybind11/numpy.h>

namespace adios2
{

class EnginePy
{

public:
    EnginePy(IO &io, const std::string &name, const Mode openMode,
             MPI_Comm mpiComm);

    ~EnginePy() = default;

    StepStatus BeginStep(const StepMode mode, const float timeoutSeconds = 0.f);

    void PutSync(VariableBase *variable, const pybind11::array &array);
    void PutSync(VariableBase *variable, const std::string &string);

    void PutDeferred(VariableBase *variable, const pybind11::array &array);
    void PutDeferred(VariableBase *variable, const std::string &string);

    void PerformPuts();

    void GetSync(VariableBase *variable, pybind11::array &array);
    void GetSync(VariableBase *variable, std::string &string);

    void GetDeferred(VariableBase *variable, pybind11::array &array);
    void GetDeferred(VariableBase *variable, std::string &string);

    void PerformGets();

    void EndStep();

    void WriteStep();

    void Close(const int transportIndex = -1);

private:
    Engine &m_Engine;
    const bool m_DebugMode;
};

} // end namespace adios2

#endif /* BINDINGS_PYTHON_SOURCE_ENGINEPY_H_ */
