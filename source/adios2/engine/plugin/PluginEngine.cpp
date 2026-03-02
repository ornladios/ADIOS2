/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginEngine.cpp
 *
 *  Created on: July 5, 2021
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 *              Caitlin Ross <caitlin.ross@kitware.com>
 */

#include "PluginEngine.h"
#include "adios2/plugin/PluginEngineInterface.h"

#include <memory>
#include <stdexcept>

#include "adios2/cxx/IO.h"
#include "adios2/cxx/Variable.h"
#include "adios2/helper/adiosLog.h"
#include "adios2/helper/adiosPluginManager.h"

#if ADIOS2_USE_MPI
#include "adios2/helper/adiosCommMPI.h"
#endif

namespace adios2
{
namespace plugin
{

/******************************************************************************/

struct PluginEngine::Impl
{
    PluginManager::EngineCreateFun m_HandleCreate;
    PluginManager::EngineDestroyFun m_HandleDestroy;
    PluginEngineInterface *m_Plugin = nullptr;
};

/******************************************************************************/

PluginEngine::PluginEngine(core::IO &io, const std::string &name, const Mode mode,
                           helper::Comm comm)
: Engine("Plugin", io, name, mode, comm.Duplicate()), m_Impl(new Impl)
{
    auto pluginNameIt = m_IO.m_Parameters.find("PluginName");
    if (pluginNameIt == m_IO.m_Parameters.end())
    {
        helper::Throw<std::runtime_error>("Plugins", "PluginEngine", "PluginEngine",
                                          "PluginName must be specified in the engine parameters");
    }

    auto pluginLibIt = m_IO.m_Parameters.find("PluginLibrary");
    if (pluginLibIt == m_IO.m_Parameters.end())
    {
        helper::Throw<std::runtime_error>(
            "Plugins", "PluginEngine", "PluginEngine",
            "PluginLibrary must be specified in the engine parameters");
    }

    auto &pluginManager = PluginManager::GetInstance();
    pluginManager.SetParameters(m_IO.m_Parameters);
    pluginManager.LoadPlugin(pluginNameIt->second, pluginLibIt->second);
    m_Impl->m_HandleCreate = pluginManager.GetEngineCreateFun(pluginNameIt->second);
    m_Impl->m_HandleDestroy = pluginManager.GetEngineDestroyFun(pluginNameIt->second);

    // Construct public API wrapper for IO to pass to the plugin
    adios2::IO publicIO(&io);

#if ADIOS2_USE_MPI
    MPI_Comm mpiComm = helper::CommAsMPI(comm);
    m_Impl->m_Plugin = m_Impl->m_HandleCreate(publicIO, pluginNameIt->second, mode, mpiComm);
#else
    m_Impl->m_Plugin = m_Impl->m_HandleCreate(publicIO, pluginNameIt->second, mode);
#endif

    m_Impl->m_Plugin->Init();
    m_IsOpen = true;
}

PluginEngine::~PluginEngine() { m_Impl->m_HandleDestroy(m_Impl->m_Plugin); }

StepStatus PluginEngine::BeginStep(StepMode mode, const float timeoutSeconds)
{
    return m_Impl->m_Plugin->BeginStep(mode, timeoutSeconds);
}

size_t PluginEngine::CurrentStep() const { return m_Impl->m_Plugin->CurrentStep(); }

void PluginEngine::PerformPuts() { m_Impl->m_Plugin->PerformPuts(); }

void PluginEngine::PerformGets() { m_Impl->m_Plugin->PerformGets(); }

void PluginEngine::EndStep() { m_Impl->m_Plugin->EndStep(); }

#define declare(T)                                                                                 \
    void PluginEngine::DoPutSync(core::Variable<T> &variable, const T *values)                     \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        m_Impl->m_Plugin->DoPutSync(pubVar, values);                                               \
    }                                                                                              \
    void PluginEngine::DoPutDeferred(core::Variable<T> &variable, const T *values)                 \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        m_Impl->m_Plugin->DoPutDeferred(pubVar, values);                                           \
    }                                                                                              \
    void PluginEngine::DoGetSync(core::Variable<T> &variable, T *values)                           \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        m_Impl->m_Plugin->DoGetSync(pubVar, values);                                               \
    }                                                                                              \
    void PluginEngine::DoGetDeferred(core::Variable<T> &variable, T *values)                       \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        m_Impl->m_Plugin->DoGetDeferred(pubVar, values);                                           \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

// Get with Selection
#define declare(T)                                                                                 \
    void PluginEngine::DoGetSync(core::Variable<T> &variable, T *values,                           \
                                 const core::Selection &sel)                                       \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        m_Impl->m_Plugin->DoGetSync(pubVar, values, sel);                                          \
    }                                                                                              \
    void PluginEngine::DoGetDeferred(core::Variable<T> &variable, T *values,                       \
                                     const core::Selection &sel)                                   \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        m_Impl->m_Plugin->DoGetDeferred(pubVar, values, sel);                                      \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

// Span-based Put
#define declare(T)                                                                                 \
    void PluginEngine::DoPut(core::Variable<T> &variable,                                          \
                             typename core::Variable<T>::Span &coreSpan, const bool initialize,    \
                             const T &value)                                                       \
    {                                                                                              \
        adios2::Variable<T> pubVar(&variable);                                                     \
        typename adios2::Variable<T>::Span pubSpan(&coreSpan);                                     \
        m_Impl->m_Plugin->DoPut(pubVar, pubSpan, initialize, value);                               \
    }
ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare)
#undef declare

MinVarInfo *PluginEngine::MinBlocksInfo(const core::VariableBase &Var, const size_t Step) const
{
    return m_Impl->m_Plugin->MinBlocksInfo(Var.m_Name, Step);
}

size_t PluginEngine::DoSteps() const { return m_Impl->m_Plugin->Steps(); }

void PluginEngine::DoClose(const int transportIndex) { m_Impl->m_Plugin->DoClose(transportIndex); }

} // end namespace plugin
} // end namespace adios2
