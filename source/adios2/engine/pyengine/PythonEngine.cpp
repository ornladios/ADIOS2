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
    pybind11::object enginePyObject;
};


/******************************************************************************/

PythonEngine::PythonEngine(IO &io, const std::string &name,
                           const OpenMode openMode, MPI_Comm mpiComm)
: Engine("Plugin", io, name, openMode, mpiComm), m_Impl(new Impl)
{
    Init();
    // m_Impl->m_Plugin =
    //     m_Impl->m_HandleCreate(io, m_Impl->m_PluginName, openMode, mpiComm);
    std::cout << "PythonEngine::PythonEngine" << std::endl;
}

PythonEngine::~PythonEngine() {
    // m_Impl->m_HandleDestroy(m_Impl->m_Plugin);
    std::cout << "PythonEngine::~PythonEngine" << std::endl;
}

void PythonEngine::PerformReads(ReadMode mode)
{
    // m_Impl->m_Plugin->PerformReads(mode);
    std::cout << "PythonEngine::PerformReads" << std::endl;
}

void PythonEngine::Release() {
    // m_Impl->m_Plugin->Release();
    std::cout << "PythonEngine::Release" << std::endl;
}

void PythonEngine::Advance(const float timeoutSeconds)
{
    // m_Impl->m_Plugin->Advance(timeoutSeconds);
    std::cout << "PythonEngine::Advance(timeoutSeconds)"
              << std::endl;
}

void PythonEngine::Advance(const AdvanceMode mode, const float timeoutSeconds)
{
    // m_Impl->m_Plugin->Advance(mode, timeoutSeconds);
    std::cout << "PythonEngine::Advance(mode, timeoutSeconds)"
              << std::endl;
}

void PythonEngine::AdvanceAsync(const AdvanceMode mode,
                                AdvanceAsyncCallback callback)
{
    // m_Impl->m_Plugin->AdvanceAsync(mode, callback);
    std::cout << "PythonEngine::AdvanceAsync" << std::endl;
}

void PythonEngine::SetCallBack(
    std::function<void(const void *, std::string, std::string, std::string,
                       std::vector<size_t>)>
        callback)
{
    // m_Impl->m_Plugin->SetCallBack(callback);
    std::cout << "PythonEngine::SetCallBack" << std::endl;
}

void PythonEngine::Close(const int transportIndex)
{
    // m_Impl->m_Plugin->Close(transportIndex);
    std::cout << "PythonEngine::Close" << std::endl;
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

    m_Impl->enginePyObject =
        adios2::PythonInstanceBuilder::BuildInstance(pluginClassName,
                                                     pluginModuleName);
}

#define define(T)                                                              \
    void PythonEngine::DoWrite(Variable<T> &variable, const T *values)   \
    {                                                                          \
        std::cout << "PythonEngine::DoWrite(var, vals), "                \
                  << "type: " << typeid(T).name() << std::endl;                \
        m_Impl->enginePyObject.attr("Writef")();                               \
    }                                                                          \
    void PythonEngine::DoScheduleRead(Variable<T> &variable,             \
                                            const T *values)                   \
    {                                                                          \
        std::cout << "PythonEngine::DoScheduledRead(var, vals), "        \
                  << "type: " << typeid(T).name() << std::endl;                \
    }                                                                          \
    void PythonEngine::DoScheduleRead(const std::string &variableName,   \
                                            const T *values)                   \
    {                                                                          \
        std::cout << "PythonEngine::DoScheduledRead(varName, vals), "    \
                  << "type: " << typeid(T).name() << std::endl;                \
    }
ADIOS2_FOREACH_TYPE_1ARG(define)
#undef define

void PythonEngine::DoWrite(VariableCompound &variable,
                                 const void *values)
{
    // m_Impl->m_Plugin->DoWrite(variable, values);
    std::cout << "PythonEngine::DoWrite" << std::endl;
}

#define define(T, L)                                                           \
    Variable<T> *PythonEngine::InquireVariable##L(                       \
        const std::string &name, const bool readIn)                            \
    {                                                                          \
        std::cout << "PythonEngine::InquireVariable##L" << std::endl;    \
        return InquireVariable##L(name, readIn);                               \
    }
ADIOS2_FOREACH_TYPE_2ARGS(define)
#undef define

VariableBase *PythonEngine::InquireVariableUnknown(
    const std::string &name, const bool readIn)
{
    std::cout << "PythonEngine::InquireVariableUnknown" << std::endl;
    return InquireVariableUnknown(name, readIn);
}

} // end namespace adios2
