/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * EnginePy.h
 *
 *  Created on: Mar 15, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_BINDINGS_PYTHON_PY11ENGINE_H_
#define ADIOS2_BINDINGS_PYTHON_PY11ENGINE_H_

#include <pybind11/numpy.h>

#include <memory> //std::shared_ptr
#include <string>

#include "adios2/core/IO.h"

#include "py11Variable.h"

namespace adios2
{
namespace py11
{

class Engine
{

public:
    Engine() = default;

    ~Engine() = default;

    StepStatus BeginStep(const StepMode mode,
                         const float timeoutSeconds = -1.f);
    StepStatus BeginStep();

    void Put(Variable variable, const pybind11::array &array,
             const Mode launch = Mode::Deferred);
    void Put(Variable variable, const std::string &string);
    void PerformPuts();

    void Get(Variable variable, pybind11::array &array,
             const Mode launch = Mode::Deferred);
    void Get(Variable variable, std::string &string,
             const Mode launch = Mode::Deferred);
    void PerformGets();

    void EndStep();

    void Flush(const int transportIndex = -1);

    void Close(const int transportIndex = -1);

    size_t CurrentStep() const;

    std::string Name() const noexcept;
    std::string Type() const noexcept;

private:
    Engine(IO io, const std::string &name, const Mode openMode,
           MPI_Comm mpiComm);

    core::Engine *m_Engine = nullptr;
    const bool m_DebugMode;
};

} // end namespace py11
} // end namespace adios2

#endif
