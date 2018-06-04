/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginEngine.h Support for an engine implemented outside libadios2
 *
 *  Created on: July 17, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#ifndef ADIOS2_ENGINE_PLUGIN_PLUGINENGINE_H_
#define ADIOS2_ENGINE_PLUGIN_PLUGINENGINE_H_

#include "PluginEngineInterface.h"

#include <functional>  // for function
#include <memory>      // for unique_ptr
#include <string>      // for string
#include <type_traits> // for add_pointer
#include <vector>      // for vector

#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/core/VariableCompound.h"

namespace adios2
{
namespace core
{
namespace engine
{

/** A front-end wrapper for an engine implemented outside of libadios2 */
class PluginEngine : public Engine
{
public:
    // Function pointers used for the plugin factory methods

    using EngineCreatePtr = std::add_pointer<PluginEngineInterface *(
        IO &, const std::string &, const Mode, MPI_Comm)>::type;
    using EngineDestroyPtr =
        std::add_pointer<void(PluginEngineInterface *)>::type;
    using EngineCreateFun =
        std::function<std::remove_pointer<EngineCreatePtr>::type>;
    using EngineDestroyFun =
        std::function<std::remove_pointer<EngineDestroyPtr>::type>;

    static void RegisterPlugin(const std::string pluginName,
                               EngineCreateFun create,
                               EngineDestroyFun destroy);
    static void RegisterPlugin(const std::string pluginName,
                               EngineCreatePtr create, EngineDestroyPtr destroy)
    {
        RegisterPlugin(pluginName, EngineCreateFun(create),
                       EngineDestroyFun(destroy));
    }

    // This is just a shortcut method to handle the case where the class type is
    // directly available to the caller so a simple new and delete call is
    // sufficient to create and destroy the engine object
    template <typename T>
    static void RegisterPlugin(const std::string name);

public:
    PluginEngine(IO &io, const std::string &name, const Mode mode,
                 MPI_Comm mpiComm);
    virtual ~PluginEngine();

    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = 0.f) override;
    void PerformPuts() override;
    void PerformGets() override;
    void EndStep() override;

protected:
    void Init() override;

#define declare(T)                                                             \
    void DoPutSync(Variable<T> &, const T *) override;                         \
    void DoPutDeferred(Variable<T> &, const T *) override;                     \
    void DoGetSync(Variable<T> &, T *) override;                               \
    void DoGetDeferred(Variable<T> &, T *) override;

    ADIOS2_FOREACH_TYPE_1ARG(declare)
#undef declare

    void DoClose(const int transportIndex = -1) override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_Impl;
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

#include "PluginEngine.inl"

#endif /* ADIOS2_ENGINE_PLUGIN_PLUGINENGINE_H_ */
