/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PythonEngine.cpp
 *
 *  Created on: Sept 21, 2017
 *      Author: Scott Wittenburg <scott.wittenburg@kitware.com>
 */

#include "PythonEngine.h"
#include "PyEngineBase.h"

#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iostream>

#include "adios2/helper/PythonInterpreter.h"
#include "adios2/helper/PythonInstanceBuilder.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace adios2
{

/******************************************************************************/

struct PythonEngine::Impl
{
    std::string m_PluginName;
    pybind11::object enginePyClass;
    pybind11::object enginePyObject;
    std::shared_ptr<PyEngineBase> eng;
};


/******************************************************************************/

PythonEngine::PythonEngine(IO &io, const std::string &name,
                           const OpenMode openMode, MPI_Comm mpiComm)
: Engine("PythonEngine", io, name, openMode, mpiComm), m_Impl(new Impl)
{
    Init();
    m_Impl->enginePyObject = m_Impl->enginePyClass("PythonEngine", io, name, openMode);
    m_Impl->eng = m_Impl->enginePyObject.cast<std::shared_ptr<PyEngineBase>>();
}

PythonEngine::~PythonEngine() {}

void PythonEngine::PerformReads(ReadMode mode)
{
    m_Impl->eng->PerformReads(mode);
}

void PythonEngine::Release() {
    m_Impl->eng->Release();
}

void PythonEngine::Advance(const float timeoutSeconds)
{
    m_Impl->eng->Advance(timeoutSeconds);
}

void PythonEngine::Advance(const AdvanceMode mode, const float timeoutSeconds)
{
    m_Impl->eng->Advance(mode, timeoutSeconds);
}

void PythonEngine::AdvanceAsync(const AdvanceMode mode,
                                AdvanceAsyncCallback callback)
{
    m_Impl->eng->AdvanceAsync(mode, callback);
}

void PythonEngine::SetCallBack(
    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
        callback)
{
    m_Impl->eng->SetCallBack(callback);
}

void PythonEngine::Close(const int transportIndex)
{
    m_Impl->eng->Close();
}

void PythonEngine::Init()
{
    auto paramPluginNameIt = m_IO.m_Parameters.find("PluginName");
    if (paramPluginNameIt == m_IO.m_Parameters.end())
    {
        throw std::invalid_argument("PythonEngine: PluginName must be "
                                    "specified in engine parameters");
    }
    m_Impl->m_PluginName = paramPluginNameIt->second;

    // Get the python engine plugin module name, if provided
    std::string* pluginModuleName = nullptr;
    auto paramPluginModuleIt = m_IO.m_Parameters.find("PluginModule");
    if (paramPluginModuleIt != m_IO.m_Parameters.end())
    {
        pluginModuleName = &(paramPluginModuleIt->second);
    }

    // Get the python engine plugin class name
    auto paramPluginClassIt = m_IO.m_Parameters.find("PluginClass");
    if (paramPluginClassIt == m_IO.m_Parameters.end())
    {
        throw std::invalid_argument("PythonEngine: PluginClass must be "
                                    "specified in engine parameters");
    }
    std::string pluginClassName = paramPluginClassIt->second;

    // Initialize python interpreter if it's not already running
    adios2::PythonInterpreter::instance().initialize();

    m_Impl->enginePyClass =
        adios2::PythonInstanceBuilder::BuildInstance(pluginClassName,
                                                     pluginModuleName);
}

#define define(T)                                                        \
    void PythonEngine::DoWrite(Variable<T> &variable, const T *values)   \
    {                                                                    \
        m_Impl->eng->DoWrite(variable, values);                          \
    }                                                                    \
                                                                         \
    void PythonEngine::DoScheduleRead(Variable<T> &variable,             \
                                            const T *values)             \
    {                                                                    \
        m_Impl->eng->DoScheduleRead(variable, values);                   \
    }                                                                    \
    void PythonEngine::DoScheduleRead(const std::string &variableName,   \
                                            const T *values)             \
    {                                                                    \
        m_Impl->eng->DoScheduleRead(variableName, values);               \
    }
ADIOS2_FOREACH_TYPE_1ARG(define)
#undef define

void PythonEngine::DoWrite(VariableCompound &variable,
                                 const void *values)
{
    std::cout << "PythonEngine::DoWrite(VariableCompound &variable, "
              << "const void *values) is not yet implemented" << std::endl;
    // m_Impl->eng->DoWrite(variable, values);
}

#define define(T, L)                                                     \
    Variable<T> *PythonEngine::InquireVariable##L(                       \
        const std::string &name, const bool readIn)                      \
    {                                                                    \
        return m_Impl->eng->InquireVariable##L(name, readIn);            \
    }
ADIOS2_FOREACH_TYPE_2ARGS(define)
#undef define

VariableBase *PythonEngine::InquireVariableUnknown(
    const std::string &name, const bool readIn)
{
    return m_Impl->eng->InquireVariableUnknown(name, readIn);
}

} // end namespace adios2
