/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ExampleWritePlugin.cpp
 *
 *  Created on: Jul 5, 2021
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 *              Caitlin Ross <caitlin.ross@kitware.com>
 */

#include "ExampleWritePlugin.h"
#include "ExampleWritePlugin.tcc"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace adios2
{
namespace plugin
{

ExampleWritePlugin::ExampleWritePlugin(adios2::IO io, const std::string &name, const Mode mode)
: PluginEngineInterface(io, name, mode)
{
}

ExampleWritePlugin::~ExampleWritePlugin()
{
    m_DataFile.close();
    m_VarFile.close();
}

void ExampleWritePlugin::Init()
{
    std::string dir = "ExamplePlugin";
    auto params = m_IO.Parameters();
    auto paramFileNameIt = params.find("DirName");
    if (paramFileNameIt != params.end())
    {
        dir = paramFileNameIt->second;
    }
#ifdef _WIN32
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0755);
#endif

    std::string fileName = dir + "/data.txt";
    m_DataFile.open(fileName);
    if (!m_DataFile)
    {
        throw std::ios_base::failure("ExampleWritePlugin: Failed to open file " + fileName);
    }

    std::string varfName = dir + "/vars.txt";
    m_VarFile.open(varfName);
    if (!m_VarFile)
    {
        throw std::ios_base::failure("ExampleWritePlugin: Failed to open file " + varfName);
    }
}

StepStatus ExampleWritePlugin::BeginStep(StepMode mode, const float timeoutSeconds)
{
    WriteVarsFromIO();
    return StepStatus::OK;
}

size_t ExampleWritePlugin::CurrentStep() const { return m_CurrentStep; }

void ExampleWritePlugin::EndStep() { m_CurrentStep++; }

#define declare(T)                                                                                 \
    void ExampleWritePlugin::DoPutSync(adios2::Variable<T> variable, const T *values)              \
    {                                                                                              \
        WriteArray(variable, values);                                                              \
    }                                                                                              \
    void ExampleWritePlugin::DoPutDeferred(adios2::Variable<T> variable, const T *values)          \
    {                                                                                              \
        WriteArray(variable, values);                                                              \
    }
ADIOS2_FOREACH_STDTYPE_1ARG(declare)
#undef declare

void ExampleWritePlugin::PerformPuts() { WriteVarsFromIO(); }

void ExampleWritePlugin::DoClose(const int transportIndex) {}

void ExampleWritePlugin::WriteVarsFromIO()
{
    auto variables = m_IO.AvailableVariables(true);
    for (const auto &vpair : variables)
    {
        const std::string &varName = vpair.first;
        const std::string varType = m_IO.VariableType(varName);
#define declare_template_instantiation(T)                                                          \
    if (varType == adios2::GetType<T>())                                                           \
    {                                                                                              \
        adios2::Variable<T> v = m_IO.InquireVariable<T>(varName);                                  \
        if (!v)                                                                                    \
            return;                                                                                \
        WriteVariableInfo(v);                                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
}

} // end namespace plugin
} // end namespace adios2

extern "C" {

adios2::plugin::ExampleWritePlugin *EngineCreate(adios2::IO io, const std::string &name,
                                                 const adios2::Mode mode)
{
    return new adios2::plugin::ExampleWritePlugin(io, name, mode);
}

void EngineDestroy(adios2::plugin::ExampleWritePlugin *obj) { delete obj; }
}
