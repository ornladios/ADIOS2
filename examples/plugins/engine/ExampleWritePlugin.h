/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ExampleWritePlugin.h Simple file writing engine. Creates directory
 * (called "ExamplePlugin" by default, but can be changed with engine parameter
 * "DirName") and writes variable info to vars.txt and actual data to data.txt.
 *
 *  Created on: Jul 5, 2021
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 *              Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef EXAMPLEWRITEPLUGIN_H_
#define EXAMPLEWRITEPLUGIN_H_

#include <fstream>
#include <memory>
#include <string>

#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/IO.h"
#include "adios2/engine/plugin/PluginEngineInterface.h"
#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace core
{
namespace engine
{

/** An engine interface to be used by the plugin infrastructure */
class ExampleWritePlugin : public PluginEngineInterface
{
public:
    ExampleWritePlugin(IO &io, const std::string &name, const Mode openMode,
                       helper::Comm comm);
    virtual ~ExampleWritePlugin();

    /** Indicates beginning of a step **/
    StepStatus BeginStep(StepMode mode,
                         const float timeoutSeconds = -1.0) override;

    /** Indicates end of a step **/
    void EndStep() override;

    /** Return the current step **/
    size_t CurrentStep() const override;

    /** Execute deferred mode Puts **/
    void PerformPuts() override;

protected:
    void Init() override;

#define declare(T)                                                             \
    void DoPutSync(Variable<T> &variable, const T *values) override;           \
    void DoPutDeferred(Variable<T> &variable, const T *values) override;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

    void DoClose(const int transportIndex = -1) override;

private:
    std::ofstream m_DataFile;
    std::ofstream m_VarFile;
    size_t m_CurrentStep = 0;

    void WriteVarsFromIO();

    template <typename T>
    void WriteVariableInfo(core::Variable<T> &variable);

    template <typename T>
    void WriteArray(Variable<T> &variable, const T *values);
};

} // end namespace engine
} // end namespace core
} // end namespace adios2

extern "C" {

adios2::core::engine::ExampleWritePlugin *
EngineCreate(adios2::core::IO &io, const std::string &name,
             const adios2::Mode mode, adios2::helper::Comm comm);
void EngineDestroy(adios2::core::engine::ExampleWritePlugin *obj);
}

#endif /* EXAMPLEWRITEPLUGIN_H_ */
