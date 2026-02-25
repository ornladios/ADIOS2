/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ExampleReadPlugin.h Simple file reading engine for reading files written by
 * the ExampleWritePlugin engine. Looks for directory
 * (called "ExamplePlugin" by default, but can be changed with engine parameter
 * "DirName") and reads variable info from vars.txt and actual data from
 * data.txt.
 *
 *  Created on: Jul 5, 2021
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef EXAMPLEREADPLUGIN_H_
#define EXAMPLEREADPLUGIN_H_

#include "plugin_engine_read_export.h"

#include <fstream>
#include <string>

#include "adios2/plugin/PluginEngineInterface.h"

namespace adios2
{
namespace plugin
{

/** An engine interface to be used by the plugin infrastructure */
class ExampleReadPlugin : public PluginEngineInterface
{
public:
    ExampleReadPlugin(adios2::IO io, const std::string &name, const Mode openMode);
    virtual ~ExampleReadPlugin();

    /** Indicates beginning of a step **/
    StepStatus BeginStep(StepMode mode = StepMode::Read,
                         const float timeoutSeconds = -1.0) override;

    /** Indicates end of a step **/
    void EndStep() override;

    /** Return the current step **/
    size_t CurrentStep() const override;

    /** Execute deferred mode Gets **/
    void PerformGets() override;

protected:
    void Init() override;

#define declare(T)                                                                                 \
    void DoGetSync(adios2::Variable<T> variable, T *values) override;                              \
    void DoGetDeferred(adios2::Variable<T> variable, T *values) override;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

    void DoClose(const int transportIndex = -1) override;

private:
    std::ifstream m_DataFile;
    std::ifstream m_VarFile;
    size_t m_CurrentStep = 0;

    template <typename T>
    void AddVariable(const std::string &name, Dims shape, Dims start, Dims count);

    template <class T>
    void ReadVariable(adios2::Variable<T> variable, T *values);
};

} // end namespace plugin
} // end namespace adios2

extern "C" {

PLUGIN_ENGINE_READ_EXPORT adios2::plugin::ExampleReadPlugin *
EngineCreate(adios2::IO io, const std::string &name, const adios2::Mode mode);
PLUGIN_ENGINE_READ_EXPORT void EngineDestroy(adios2::plugin::ExampleReadPlugin *obj);
}

#endif /* EXAMPLEREADPLUGIN_H_ */
