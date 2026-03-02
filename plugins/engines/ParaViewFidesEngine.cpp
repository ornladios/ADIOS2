/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ParaViewFidesEngine.cpp
 *
 *  Created on: Sep 21, 2022
 *      Author: Caitlin Ross <caitlin.ross@kitware.com>
 */

#include "ParaViewFidesEngine.h"

#include <adios2.h>

#include <catalyst.hpp>

#include <iostream>
#include <sstream>

namespace fides_plugin
{

struct ParaViewFidesEngine::EngineImpl
{
    adios2::ADIOS Adios;
    adios2::IO Io;
    adios2::Engine Writer;

    std::string ScriptFileName;
    std::string JSONFileName;

    int Rank = 0;

    EngineImpl()
    {
        this->Io = this->Adios.DeclareIO("InlinePluginIO");
        this->Io.SetEngine("inline");
        this->Writer = this->Io.Open("write", adios2::Mode::Write);
    }

    void CatalystConfig()
    {
        std::cout << "\tCatalyst Library Version: " << CATALYST_VERSION << "\n";
        std::cout << "\tCatalyst ABI Version: " << CATALYST_ABI_VERSION << "\n";

        conduit_cpp::Node node;
        catalyst_about(conduit_cpp::c_node(&node));
        auto implementation = node.has_path("catalyst/implementation")
                                  ? node["catalyst/implementation"].as_string()
                                  : std::string("stub");
        std::cout << "\tImplementation: " << implementation << "\n\n";
    }

    void CatalystInit()
    {
        conduit_cpp::Node node;
        node["catalyst/scripts/script/filename"].set(this->ScriptFileName);

        // options to set up the fides reader in paraview
        std::ostringstream address;
        address << &Io;

        node["catalyst/fides/json_file"].set(this->JSONFileName);
        node["catalyst/fides/data_source_io/source"].set(std::string("source"));
        node["catalyst/fides/data_source_io/address"].set(address.str());
        node["catalyst/fides/data_source_path/source"].set(std::string("source"));
        node["catalyst/fides/data_source_path/path"].set(std::string("DataReader"));
        catalyst_initialize(conduit_cpp::c_node(&node));

        if (this->Rank == 0)
        {
            this->CatalystConfig();
        }
    }

    void CatalystExecute()
    {
        auto timestep = this->Writer.CurrentStep();
        conduit_cpp::Node node;
        node["catalyst/state/timestep"].set(timestep);
        // catalyst requires the next one, but when using Fides as the reader
        // for Catalyst, it will grab the time from the correct adios variable
        // if it is specified in the data model
        node["catalyst/state/time"].set(timestep);
        node["catalyst/channels/fides/type"].set(std::string("fides"));

        // options to set up the fides reader in paraview
        std::ostringstream address;
        address << &Io;

        node["catalyst/fides/json_file"].set(this->JSONFileName);
        node["catalyst/fides/data_source_io/source"].set(std::string("source"));
        node["catalyst/fides/data_source_io/address"].set(address.str());
        node["catalyst/fides/data_source_path/source"].set(std::string("source"));
        node["catalyst/fides/data_source_path/path"].set(std::string("DataReader"));

        // catalyst requires the data node on a channel, but we don't actually
        // need it when using fides, so just create a dummy object to pass
        // the validation in catalyst
        conduit_cpp::Node dummy;
        dummy["dummy"].set(0);
        node["catalyst/channels/fides/data"].set(dummy);

        catalyst_execute(conduit_cpp::c_node(&node));
    }
};

ParaViewFidesEngine::ParaViewFidesEngine(adios2::IO io, const std::string &name, adios2::Mode mode)
: adios2::plugin::PluginEngineInterface(io, name, mode), Impl(new EngineImpl)
{
    // Need to define the Variables in the IO object used for the inline engine
    auto variables = io.AvailableVariables(true);
    for (const auto &it : variables)
    {
        const std::string varType = io.VariableType(it.first);
#define declare_type(T)                                                                            \
    if (varType == adios2::GetType<T>())                                                           \
    {                                                                                              \
        adios2::Variable<T> srcVar = io.InquireVariable<T>(it.first);                              \
        this->Impl->Io.DefineVariable<T>(it.first, srcVar.Shape(), srcVar.Start(),                 \
                                         srcVar.Count());                                          \
        continue;                                                                                  \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    auto params = io.Parameters();
    const auto &scriptIt = params.find("Script");
    if (scriptIt != params.end())
    {
        this->Impl->ScriptFileName = scriptIt->second;
    }

    // TODO required for now, but support data model generation in the future
    const auto &fileIt = params.find("DataModel");
    if (fileIt == params.end())
    {
        throw std::runtime_error("couldn't find DataModel in parameters!");
    }
    this->Impl->JSONFileName = fileIt->second;

    this->Impl->CatalystInit();
}

ParaViewFidesEngine::~ParaViewFidesEngine()
{
    conduit_cpp::Node node;
    catalyst_finalize(conduit_cpp::c_node(&node));
}

adios2::StepStatus ParaViewFidesEngine::BeginStep(adios2::StepMode mode, const float timeoutSeconds)
{
    return this->Impl->Writer.BeginStep(mode, timeoutSeconds);
}

size_t ParaViewFidesEngine::CurrentStep() const { return this->Impl->Writer.CurrentStep(); }

void ParaViewFidesEngine::EndStep()
{
    this->Impl->Writer.EndStep();

    this->Impl->CatalystExecute();
}

void ParaViewFidesEngine::PerformPuts() { this->Impl->Writer.PerformPuts(); }

#define declare(T)                                                                                 \
    void ParaViewFidesEngine::DoPutSync(adios2::Variable<T> variable, const T *values)             \
    {                                                                                              \
        adios2::Variable<T> inlineVar = this->Impl->Io.InquireVariable<T>(variable.Name());        \
        this->Impl->Writer.Put(inlineVar, values, adios2::Mode::Sync);                             \
    }                                                                                              \
    void ParaViewFidesEngine::DoPutDeferred(adios2::Variable<T> variable, const T *values)         \
    {                                                                                              \
        adios2::Variable<T> inlineVar = this->Impl->Io.InquireVariable<T>(variable.Name());        \
        this->Impl->Writer.Put(inlineVar, values);                                                 \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

void ParaViewFidesEngine::DoClose(const int transportIndex) { this->Impl->Writer.Close(); }

} // end namespace fides_plugin

extern "C" {

fides_plugin::ParaViewFidesEngine *EngineCreate(adios2::IO io, const std::string &name,
                                                const adios2::Mode mode)
{
    return new fides_plugin::ParaViewFidesEngine(io, name, mode);
}

void EngineDestroy(fides_plugin::ParaViewFidesEngine *obj) { delete obj; }
}
