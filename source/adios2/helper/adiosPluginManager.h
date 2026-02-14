/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosPluginManager.h
 *
 * Created on: Dec 14, 2021
 *     Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef ADIOS2_HELPER_PLUGINMANAGER_H
#define ADIOS2_HELPER_PLUGINMANAGER_H

#include "adios2/plugin/PluginOperatorInterface.h"

#include "adios2/cxx/IO.h"

#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

namespace adios2
{
namespace plugin
{

class PluginEngineInterface; // forward declare

class PluginManager
{
public:
#if ADIOS2_USE_MPI
    using EngineCreatePtr = std::add_pointer_t<PluginEngineInterface *(
        adios2::IO, const std::string &, const Mode, MPI_Comm)>;
#else
    using EngineCreatePtr =
        std::add_pointer_t<PluginEngineInterface *(adios2::IO, const std::string &, const Mode)>;
#endif
    using EngineDestroyPtr = std::add_pointer_t<void(PluginEngineInterface *)>;
    using EngineCreateFun = std::function<std::remove_pointer_t<EngineCreatePtr>>;
    using EngineDestroyFun = std::function<std::remove_pointer_t<EngineDestroyPtr>>;

    using OperatorCreatePtr = std::add_pointer_t<PluginOperatorInterface *(const Params &)>;
    using OperatorDestroyPtr = std::add_pointer_t<void(PluginOperatorInterface *)>;
    using OperatorCreateFun = std::function<std::remove_pointer_t<OperatorCreatePtr>>;
    using OperatorDestroyFun = std::function<std::remove_pointer_t<OperatorDestroyPtr>>;

    static PluginManager &GetInstance();

    void SetParameters(const Params &params);

    /**
     * Attempts to load a single plugin specified by pluginName and
     * pluginLibrary.
     */
    bool LoadPlugin(const std::string &pluginName, const std::string &pluginLibrary);

    EngineCreateFun GetEngineCreateFun(const std::string &name);
    EngineDestroyFun GetEngineDestroyFun(const std::string &name);

    OperatorCreateFun GetOperatorCreateFun(const std::string &name);
    OperatorDestroyFun GetOperatorDestroyFun(const std::string &name);

private:
    PluginManager();
    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;
    virtual ~PluginManager();

    static void CreateInstance();

    bool OpenPlugin(const std::string &pluginName, const std::string &pluginLibrary,
                    const std::string &pluginPath);

    static PluginManager *m_Instance;
    static bool m_Destroyed;

    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // end namespace plugin
} // end namespace adios2

#endif /* ADIOS2_HELPER_PLUGINMANAGER_H */
