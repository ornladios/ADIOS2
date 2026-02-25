/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ParaViewFidesEngine.h
 *
 *  Created on: Sep 21, 2022
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#ifndef PARAVIEWFIDESENGINE_H
#define PARAVIEWFIDESENGINE_H

#include "adios2/plugin/PluginEngineInterface.h"

#include <memory>

namespace fides_plugin
{

/**
 *  ParaViewFidesEngine: An engine plugin for ADIOS2 that supports in situ
 *  visualization with the Inline engine. This engine handles the writing
 *  side of things by forwarding Puts onto the Inline writer.
 *  To handle to read side of things, a ParaView Fides reader is set up
 *  so the written data can be used in a visualization pipeline.
 *  Finally, the engine uses ParaView's catalyst infrastructure to run
 *  python scripts that set up the visualization pipeline.
 *
 *  Parameters to be passed to ADIOS to use this engine:
 *  Key -> Value
 *  "Script" -> "/path/to/python/script"
 *      - Python script that sets up ParaView pipeline for visualization
 *  "DataModel" -> "/path/to/json"
 *      - Fides JSON data model file
 *
 */
class ParaViewFidesEngine : public adios2::plugin::PluginEngineInterface
{

public:
    ParaViewFidesEngine(adios2::IO io, const std::string &name, adios2::Mode mode);

    ~ParaViewFidesEngine() override;

    adios2::StepStatus BeginStep(adios2::StepMode mode, const float timeoutSeconds = -1.0) override;

    void EndStep() override;

    size_t CurrentStep() const override;

    void PerformPuts() override;

protected:
#define declare_type(T)                                                                            \
    void DoPutSync(adios2::Variable<T>, const T *) override;                                       \
    void DoPutDeferred(adios2::Variable<T>, const T *) override;
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

    void DoClose(const int transportIndex = -1) override;

private:
    struct EngineImpl;
    std::unique_ptr<EngineImpl> Impl;
};

} // end namespace fides_plugin

extern "C" {

fides_plugin::ParaViewFidesEngine *EngineCreate(adios2::IO io, const std::string &name,
                                                const adios2::Mode mode);
void EngineDestroy(fides_plugin::ParaViewFidesEngine *obj);
}

#endif // PARAVIEWFIDESENGINE_H
