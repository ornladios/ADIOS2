/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PyEngineBase.h: This is the base class used to translate the protected
 * template functions into public members that can be overridden in python.
 *
 *  Created on: Oct 9, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
// #include <pybind11/stl.h>

// #include "adios2/core/Engine.h"
// #include "adios2/core/IO.h"
// #include "adios2/core/Variable.h"

#ifndef ADIOS2_ENGINE_PYENGINE_PYENGINEBASE_H_
#define ADIOS2_ENGINE_PYENGINE_PYENGINEBASE_H_

namespace adios2
{

class PyEngineBase : public Engine
{
    // Make sure PythonEngine can forward all its calls here
    friend class PythonEngine;

public:
    PyEngineBase(const std::string engineType, IO &io,
                 const std::string &name, const OpenMode openMode)
    : Engine(engineType, io, name, openMode, io.m_MPIComm)
    {
    }

    virtual ~PyEngineBase() = default;

    using Engine::Init;
    virtual void DoWrite(VariableBase *var, pybind11::array values) = 0;
    using Engine::Close;

protected:
    // The C++ code calls this implementation which will redirect to the
    // python implementation.
#define define_dowrite(T)                                                      \
    void DoWrite(Variable<T> &var, const T *values) override                   \
    {                                                                          \
        DoWrite(&var,                                                          \
                pybind11::array_t<T>(std::accumulate(var.m_Count.begin(),      \
                                                     var.m_Count.end(), 0),    \
                                     values));                                 \
    }
        ADIOS2_FOREACH_TYPE_1ARG(define_dowrite)
#undef define_dowrite
};
}

#endif /* ADIOS2_ENGINE_PYENGINE_PYENGINEBASE_H_ */
