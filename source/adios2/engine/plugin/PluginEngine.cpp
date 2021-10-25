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
#include "PluginEngineInterface.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

#include "adios2/helper/adiosDynamicBinder.h"

#include <adios2sys/SystemTools.hxx>

namespace adios2
{
namespace core
{
namespace engine
{

/******************************************************************************/

struct PluginEngine::Impl
{
    std::string m_PluginName = "UserPlugin";
    std::unique_ptr<helper::DynamicBinder> m_Binder;
    EngineCreateFun m_HandleCreate;
    EngineDestroyFun m_HandleDestroy;
    PluginEngineInterface *m_Plugin = nullptr;
};

/******************************************************************************/

PluginEngine::PluginEngine(IO &io, const std::string &name, const Mode mode,
                           helper::Comm comm)
: Engine("Plugin", io, name, mode, comm.Duplicate()), m_Impl(new Impl)
{
    Init();
    m_Impl->m_Plugin = m_Impl->m_HandleCreate(io, m_Impl->m_PluginName, mode,
                                              comm.Duplicate());
}

PluginEngine::~PluginEngine() { m_Impl->m_HandleDestroy(m_Impl->m_Plugin); }

StepStatus PluginEngine::BeginStep(StepMode mode, const float timeoutSeconds)
{
    return m_Impl->m_Plugin->BeginStep(mode, timeoutSeconds);
}

void PluginEngine::PerformPuts() { m_Impl->m_Plugin->PerformPuts(); }

void PluginEngine::PerformGets() { m_Impl->m_Plugin->PerformGets(); }

void PluginEngine::EndStep() { m_Impl->m_Plugin->EndStep(); }

void PluginEngine::Init()
{
    auto paramPluginNameIt = m_IO.m_Parameters.find("PluginName");
    if (paramPluginNameIt != m_IO.m_Parameters.end())
    {
        m_Impl->m_PluginName = paramPluginNameIt->second;
    }

    std::string pluginPath;
    adios2sys::SystemTools::GetEnv("ADIOS2_PLUGIN_PATH", pluginPath);

    auto paramPluginLibraryIt = m_IO.m_Parameters.find("PluginLibrary");
    if (paramPluginLibraryIt == m_IO.m_Parameters.end())
    {
        throw std::invalid_argument(
            "PluginEngine: PluginLibrary must be specified in "
            "engine parameters if no PluginName "
            "is specified");
    }
    std::string &pluginLibrary = paramPluginLibraryIt->second;

    m_Impl->m_Binder.reset(
        new helper::DynamicBinder(pluginLibrary, pluginPath));

    m_Impl->m_HandleCreate = reinterpret_cast<EngineCreatePtr>(
        m_Impl->m_Binder->GetSymbol("EngineCreate"));
    if (!m_Impl->m_HandleCreate)
    {
        throw std::runtime_error("PluginEngine: Unable to locate "
                                 "EngineCreate symbol in specified plugin "
                                 "library");
    }

    m_Impl->m_HandleDestroy = reinterpret_cast<EngineDestroyPtr>(
        m_Impl->m_Binder->GetSymbol("EngineDestroy"));
    if (!m_Impl->m_HandleDestroy)
    {
        throw std::runtime_error("PluginEngine: Unable to locate "
                                 "EngineDestroy symbol in specified plugin "
                                 "library");
    }
}

#define declare(T)                                                             \
    void PluginEngine::DoPutSync(Variable<T> &variable, const T *values)       \
    {                                                                          \
        m_Impl->m_Plugin->DoPutSync(variable, values);                         \
    }                                                                          \
    void PluginEngine::DoPutDeferred(Variable<T> &variable, const T *values)   \
    {                                                                          \
        m_Impl->m_Plugin->DoPutDeferred(variable, values);                     \
    }                                                                          \
    void PluginEngine::DoGetSync(Variable<T> &variable, T *values)             \
    {                                                                          \
        m_Impl->m_Plugin->DoGetSync(variable, values);                         \
    }                                                                          \
    void PluginEngine::DoGetDeferred(Variable<T> &variable, T *values)         \
    {                                                                          \
        m_Impl->m_Plugin->DoGetDeferred(variable, values);                     \
    }

ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

void PluginEngine::DoClose(const int transportIndex)
{
    m_Impl->m_Plugin->Close(transportIndex);
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
