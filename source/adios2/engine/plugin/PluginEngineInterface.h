/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginEngineInterface.h Engines using the plugin interface should derive from
 * this class.
 *
 *  Created on: July 17, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#ifndef ADIOS2_ENGINE_PLUGIN_PLUGINENGINEINTERFACE_H_
#define ADIOS2_ENGINE_PLUGIN_PLUGINENGINEINTERFACE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"

namespace adios2
{

/** An engine interface to be used aby the plugin infrastructure */
class PluginEngineInterface : public Engine
{
    // Give the plugin engine access to everything
    friend class PluginEngine;

public:
    PluginEngineInterface(IO &io, const std::string &name,
                          const OpenMode openMode, MPI_Comm mpiComm);
    virtual ~PluginEngineInterface() = default;
};

} // end namespace adios

#endif /* ADIOS2_ENGINE_PLUGIN_PLUGINENGINEINTERFACE_H_ */
