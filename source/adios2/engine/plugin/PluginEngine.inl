/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginEngine.h Support for an engine implemented outside libadios2
 *
 *  Created on: July 17, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#ifndef ADIOS2_ENGINE_PLUGIN_ENGINE_INL_
#define ADIOS2_ENGINE_PLUGIN_ENGINE_INL_
#ifndef ADIOS2_ENGINE_PLUGIN_PLUGINENGINE_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

#include "PluginEngine.h"

namespace adios2
{

template <typename T>
void PluginEngine::RegisterPlugin(const std::string name)
{
    EngineCreateFun createFun =
        [](IO &io, const std::string &name, const Mode openMode,
           MPI_Comm mpiComm) -> PluginEngineInterface * {
        return new T(io, name, openMode, mpiComm);
    };
    EngineDestroyFun destroyFun = [](Engine *obj) -> void { delete obj; };

    RegisterPlugin(name, createFun, destroyFun);
}
} // end namespace adios2

#endif // ADIOS2_ENGINE_PLUGIN_ENGINE_INL_
