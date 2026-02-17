/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginEngineInterface.h Engines using the plugin interface should derive from
 * this class. External plugin engines only need to include this header and
 * link against adios2_cxx (not adios2_core).
 *
 *  Created on: July 5, 2021
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 *              Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef ADIOS2_ENGINE_PLUGIN_PLUGINENGINEINTERFACE_H_
#define ADIOS2_ENGINE_PLUGIN_PLUGINENGINEINTERFACE_H_

/* Plugin engine interface version.
 * v1: derived from core::Engine (ADIOS2 <= 2.11)
 * v2: standalone, uses public C++ API only (ADIOS2 >= 2.12)
 */
#define ADIOS2_PLUGIN_ENGINE_INTERFACE 2

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/cxx/IO.h"
#include "adios2/cxx/Variable.h"

#if ADIOS2_USE_MPI
#include <mpi.h>
#endif

#include <string>

namespace adios2
{
namespace plugin
{

/** An engine interface to be used by the plugin infrastructure.
 *  Plugin engines inherit from this standalone class and interact with
 *  ADIOS2 only through the public C++ API (adios2::IO, adios2::Variable<T>).
 */
class PluginEngineInterface
{
public:
#if ADIOS2_USE_MPI
    PluginEngineInterface(adios2::IO io, const std::string &name, const Mode mode, MPI_Comm comm)
    : m_IO(io), m_Name(name), m_OpenMode(mode), m_Comm(comm)
    {
    }
#endif

    PluginEngineInterface(adios2::IO io, const std::string &name, const Mode mode)
    : m_IO(io), m_Name(name), m_OpenMode(mode)
    {
    }

    virtual ~PluginEngineInterface() = default;

    // === Step control (must implement) ===
    virtual StepStatus BeginStep(StepMode mode, const float timeoutSeconds = -1.0f) = 0;
    virtual void EndStep() = 0;
    virtual size_t CurrentStep() const = 0;

    // === Deferred execution (override if needed) ===
    virtual void PerformPuts() {}
    virtual void PerformGets() {}

    // === Per-type Put/Get (override the types you support) ===
#define declare(T)                                                                                 \
    virtual void DoPutSync(adios2::Variable<T> variable, const T *values) {}                       \
    virtual void DoPutDeferred(adios2::Variable<T> variable, const T *values) {}                   \
    virtual void DoGetSync(adios2::Variable<T> variable, T *values) {}                             \
    virtual void DoGetDeferred(adios2::Variable<T> variable, T *values) {}
    ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

    // === Close ===
    virtual void DoClose(const int transportIndex = -1) {}

    // === Init (called after construction, override if needed) ===
    virtual void Init() {}

protected:
    adios2::IO m_IO;
    std::string m_Name;
    Mode m_OpenMode;
#if ADIOS2_USE_MPI
    MPI_Comm m_Comm = MPI_COMM_NULL;
#endif
};

} // end namespace plugin
} // end namespace adios2

#endif /* ADIOS2_ENGINE_PLUGIN_PLUGINENGINEINTERFACE_H_ */
