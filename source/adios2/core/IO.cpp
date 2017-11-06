/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IO.cpp
 *
 *  Created on: Jan 6, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "IO.h"
#include "IO.tcc"

#include <sstream>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/engine/bp/BPFileReader.h"
#include "adios2/engine/bp/BPFileWriter.h"
#include "adios2/engine/plugin/PluginEngine.h"
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap

#ifdef ADIOS2_HAVE_DATAMAN // external dependencies
#include "adios2/engine/dataman/DataManReader.h"
#include "adios2/engine/dataman/DataManWriter.h"
#endif

#ifdef ADIOS2_HAVE_ADIOS1 // external dependencies
#include "adios2/engine/adios1/ADIOS1Reader.h"
#include "adios2/engine/adios1/ADIOS1Writer.h"
#endif

#ifdef ADIOS2_HAVE_HDF5 // external dependencies
#include "adios2/engine/hdf5/HDF5ReaderP.h"
#include "adios2/engine/hdf5/HDF5WriterP.h"
#if H5_VERSION_GE(1, 11, 0)
#include "adios2/engine/mixer/HDFMixer.h"
#endif
#endif

namespace adios2
{

IO::IO(const std::string name, MPI_Comm mpiComm, const bool inConfigFile,
       const std::string hostLanguage, const bool debugMode)
: m_Name(name), m_MPIComm(mpiComm), m_InConfigFile(inConfigFile),
  m_HostLanguage(hostLanguage), m_DebugMode(debugMode)
{
}

void IO::SetEngine(const std::string engineType) noexcept
{
    m_EngineType = engineType;
}
void IO::SetIOMode(const IOMode ioMode) { m_IOMode = ioMode; };

void IO::SetParameters(const Params &parameters) noexcept
{
    m_Parameters.clear();

    for (const auto &parameter : parameters)
    {
        m_Parameters[parameter.first] = parameter.second;
    }
}

void IO::SetParameter(const std::string key, const std::string value) noexcept
{
    m_Parameters[key] = value;
}

Params &IO::GetParameters() noexcept { return m_Parameters; }

unsigned int IO::AddTransport(const std::string type, const Params &parameters)
{
    Params parametersMap(parameters);
    if (m_DebugMode)
    {
        if (parameters.count("transport") == 1 ||
            parameters.count("Transport") == 1)
        {
            throw std::invalid_argument("ERROR: key Transport (or transport) "
                                        "is not valid for transport type " +
                                        type + ", in call to AddTransport)");
        }

        CheckTransportType(type);
    }

    parametersMap["transport"] = type;
    m_TransportsParameters.push_back(parametersMap);
    return static_cast<unsigned int>(m_TransportsParameters.size() - 1);
}

void IO::AddOperator(Operator &adiosOperator, const Params &parameters) noexcept
{
    m_Operators.push_back(OperatorInfo{adiosOperator, parameters});
}

void IO::SetTransportParameter(const unsigned int transportIndex,
                               const std::string key, const std::string value)
{
    if (m_DebugMode)
    {
        if (transportIndex >=
            static_cast<unsigned int>(m_TransportsParameters.size()))
        {
            throw std::invalid_argument(
                "ERROR: transportIndex is larger than "
                "transports created with AddTransport, for key: " +
                key + ", value: " + value + "in call to SetTransportParameter "
                                            "\n");
        }
    }

    m_TransportsParameters[transportIndex][key] = value;
}

const DataMap &IO::GetVariablesDataMap() const noexcept { return m_Variables; }

const DataMap &IO::GetAttributesDataMap() const noexcept
{
    return m_Attributes;
}

bool IO::InConfigFile() const { return m_InConfigFile; };

bool IO::RemoveVariable(const std::string &name) noexcept
{
    bool isRemoved = false;
    auto itVariable = m_Variables.find(name);
    // variable exists
    if (itVariable != m_Variables.end())
    {
        // first remove the Variable object
        const std::string type(itVariable->second.first);
        const unsigned int index(itVariable->second.second);

        if (type == "compound")
        {
            auto variableMap = m_Compound;
            variableMap.erase(index);
        }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        auto variableMap = GetVariableMap<T>();                                \
        variableMap.erase(index);                                              \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

        isRemoved = true;
    }

    if (isRemoved)
    {
        m_Variables.erase(name);
    }

    return isRemoved;
}

std::map<std::string, Params> IO::GetAvailableVariables() noexcept
{
    std::map<std::string, Params> variablesInfo;
    for (const auto &variablePair : m_Variables)
    {
        const std::string name(variablePair.first);
        const std::string type(variablePair.second.first);
        variablesInfo[name]["Type"] = type;

        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        Variable<T> &variable = *InquireVariable<T>(name);                     \
        std::ostringstream minSS;                                              \
        minSS << variable.m_Min;                                               \
        variablesInfo[name]["Min"] = minSS.str();                              \
        std::ostringstream maxSS;                                              \
        maxSS << variable.m_Max;                                               \
        variablesInfo[name]["Max"] = maxSS.str();                              \
        variablesInfo[name]["StepsStart"] =                                    \
            std::to_string(variable.m_AvailableStepsStart);                    \
        variablesInfo[name]["StepsCount"] =                                    \
            std::to_string(variable.m_AvailableStepsCount);                    \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
    // TODO: add dimensions
    return variablesInfo;
}

std::string IO::InquireVariableType(const std::string &name) const noexcept
{
    auto itVariable = m_Variables.find(name);
    if (itVariable == m_Variables.end())
    {
        return std::string();
    }

    return itVariable->second.first;
}

Engine &IO::Open(const std::string &name, const Mode mode, MPI_Comm mpiComm)
{
    if (m_DebugMode)
    {
        if (m_Engines.count(name) == 1)
        {
            throw std::invalid_argument("ERROR: IO Engine with name " + name +
                                        " already created, in call to Open.\n");
        }
    }

    std::shared_ptr<Engine> engine;

    const bool isDefaultWriter =
        m_EngineType.empty() &&
                (mode == Mode::Write || mode == Mode::Append)
            ? true
            : false;

    const bool isDefaultReader =
        m_EngineType.empty() && (mode == Mode::Read) ? true : false;

    if (isDefaultWriter || m_EngineType == "BPFileWriter")
    {
        engine = std::make_shared<BPFileWriter>(*this, name, mode, mpiComm);
    }
    else if (isDefaultReader || m_EngineType == "BPFileReader")
    {
        engine = std::make_shared<BPFileReader>(*this, name, mode, mpiComm);
    }
    else if (m_EngineType == "HDFMixer")
    {
#ifdef ADIOS2_HAVE_HDF5
#if H5_VERSION_GE(1, 11, 0)
        engine = std::make_shared<HDFMixer>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: update HDF5 >= 1.11 to support VDS.");
#endif
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else if (m_EngineType == "DataManWriter")
    {
#ifdef ADIOS2_HAVE_DATAMAN
        engine =
            std::make_shared<DataManWriter>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't Open DataManWriter\n");
#endif
    }
    else if (m_EngineType == "DataManReader")
    {
#ifdef ADIOS2_HAVE_DATAMAN
        engine =
            std::make_shared<DataManReader>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't Open DataManReader\n");
#endif
    }
    else if (m_EngineType == "ADIOS1Writer")
    {
#ifdef ADIOS2_HAVE_ADIOS1
        engine = std::make_shared<ADIOS1Writer>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with ADIOS "
            "1.x library, can't Open ADIOS1Writer\n");
#endif
    }
    else if (m_EngineType == "ADIOS1Reader")
    {
#ifdef ADIOS2_HAVE_ADIOS1
        engine = std::make_shared<ADIOS1Reader>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with ADIOS "
            "1.x library, can't Open ADIOS1Reader\n");
#endif
    }
    else if (m_EngineType == "HDF5Writer")
    {
#ifdef ADIOS2_HAVE_HDF5
        engine = std::make_shared<HDF5WriterP>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else if (m_EngineType == "HDF5Reader")
    {
#ifdef ADIOS2_HAVE_HDF5
        engine = std::make_shared<HDF5ReaderP>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else if (m_EngineType == "PluginEngine")
    {
        engine = std::make_shared<PluginEngine>(*this, name, mode, mpiComm);
    }
    else
    {
        if (m_DebugMode)
        {
            throw std::invalid_argument("ERROR: engine " + m_EngineType +
                                        " not supported, IO SetEngine must add "
                                        "a supported engine, in call to "
                                        "Open\n");
        }
    }

    auto itEngine = m_Engines.emplace(name, std::move(engine));

    if (m_DebugMode)
    {
        if (!itEngine.second)
        {
            throw std::invalid_argument(
                "ERROR: engine of type " + m_EngineType + " and name " + name +
                " could not be created, in call to Open\n");
        }
    }
    // return a reference
    return *itEngine.first->second.get();
}

Engine &IO::Open(const std::string &name, const Mode openMode)
{
    return Open(name, openMode, m_MPIComm);
}

// PRIVATE
int IO::GetMapIndex(const std::string &name, const DataMap &dataMap) const
    noexcept
{
    auto itName = dataMap.find(name);
    if (itName == dataMap.end())
    {
        return -1;
    }
    return itName->second.second;
}

void IO::CheckAttributeCommon(const std::string &name) const
{
    auto itAttribute = m_Attributes.find(name);
    if (!IsEnd(itAttribute, m_Attributes))
    {
        throw std::invalid_argument("ERROR: attribute " + name +
                                    " exists in IO object " + m_Name +
                                    ", in call to DefineAttribute\n");
    }
}

bool IO::IsEnd(DataMap::const_iterator itDataMap, const DataMap &dataMap) const
{
    if (itDataMap == dataMap.end())
    {
        return true;
    }
    return false;
}

void IO::CheckTransportType(const std::string type) const
{
    if (type.empty() || type.find("=") != type.npos)
    {
        throw std::invalid_argument(
            "ERROR: wrong first argument " + type +
            ", must "
            "be a single word for a supported transport type, in "
            "call to IO AddTransport \n");
    }
}

// Explicitly instantiate the necessary public template implementations
#define define_template_instantiation(T)                                       \
    template Variable<T> &IO::DefineVariable<T>(                               \
        const std::string &, const Dims &, const Dims &, const Dims &,         \
        const bool, T *);                                                      \
    template Variable<T> *IO::InquireVariable<T>(const std::string &) noexcept;

ADIOS2_FOREACH_TYPE_1ARG(define_template_instantiation)
#undef define_template_instatiation

#define declare_template_instantiation(T)                                      \
    template Attribute<T> &IO::DefineAttribute<T>(const std::string &,         \
                                                  const T *, const size_t);    \
    template Attribute<T> &IO::DefineAttribute<T>(const std::string &,         \
                                                  const T &);                  \
    template Attribute<T> *IO::InquireAttribute<T>(                            \
        const std::string &) noexcept;

ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios
