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
#include "adios2/engine/skeleton/SkeletonReader.h"
#include "adios2/engine/skeleton/SkeletonWriter.h"
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap

#ifdef ADIOS2_HAVE_DATAMAN // external dependencies
#include "adios2/engine/dataman/DataManReader.h"
#include "adios2/engine/dataman/DataManWriter.h"
#endif

#ifdef ADIOS2_HAVE_SST // external dependencies
#include "adios2/engine/sst/SstReader.h"
#include "adios2/engine/sst/SstWriter.h"
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

#ifdef ADIOS2_HAVE_MPI // external dependencies
#include "adios2/engine/insitumpi/InSituMPIReader.h"
#include "adios2/engine/insitumpi/InSituMPIWriter.h"
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
                key + ", value: " + value +
                "in call to SetTransportParameter\n");
        }
    }

    m_TransportsParameters[transportIndex][key] = value;
}

const DataMap &IO::GetVariablesDataMap() const noexcept { return m_Variables; }

const DataMap &IO::GetAttributesDataMap() const noexcept
{
    return m_Attributes;
}

bool IO::InConfigFile() const noexcept { return m_InConfigFile; };

void IO::SetDeclared() noexcept { m_IsDeclared = true; };

bool IO::IsDeclared() const noexcept { return m_IsDeclared; }

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
        isRemoved = true;                                                      \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }

    if (isRemoved)
    {
        m_Variables.erase(name);
    }

    return isRemoved;
}

void IO::RemoveAllVariables() noexcept
{
    m_Variables.clear();
#define declare_type(T) GetVariableMap<T>().clear();
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    m_Compound.clear();
}

bool IO::RemoveAttribute(const std::string &name) noexcept
{
    bool isRemoved = false;
    auto itAttribute = m_Attributes.find(name);
    // attribute exists
    if (itAttribute != m_Attributes.end())
    {
        // first remove the Variable object
        const std::string type(itAttribute->second.first);
        const unsigned int index(itAttribute->second.second);

        if (type.empty())
        {
            // nothing to do
        }
#define declare_type(T)                                                        \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        auto variableMap = GetVariableMap<T>();                                \
        variableMap.erase(index);                                              \
        isRemoved = true;                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
    }

    if (isRemoved)
    {
        m_Attributes.erase(name);
    }

    return isRemoved;
}

void IO::RemoveAllAttributes() noexcept
{
    m_Attributes.clear();

#define declare_type(T) GetAttributeMap<T>().clear();
    ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_type)
#undef declare_type
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
        variablesInfo[name]["AvailableStepsCount"] =                           \
            ValueToString(variable.m_AvailableStepsCount);                     \
        variablesInfo[name]["Shape"] = VectorToCSV(variable.m_Shape);          \
        if (variable.m_SingleValue)                                            \
        {                                                                      \
            variablesInfo[name]["SingleValue"] = "true";                       \
            variablesInfo[name]["Value"] = ValueToString(variable.m_Value);    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            variablesInfo[name]["SingleValue"] = "false";                      \
            variablesInfo[name]["Min"] = ValueToString(variable.m_Min);        \
            variablesInfo[name]["Max"] = ValueToString(variable.m_Max);        \
        }                                                                      \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    return variablesInfo;
}

std::map<std::string, Params> IO::GetAvailableAttributes() noexcept
{
    std::map<std::string, Params> attributesInfo;
    for (const auto &attributePair : m_Attributes)
    {
        const std::string name(attributePair.first);
        const std::string type(attributePair.second.first);
        attributesInfo[name]["Type"] = type;

        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == GetType<T>())                                             \
    {                                                                          \
        Attribute<T> &attribute = *InquireAttribute<T>(name);                  \
        attributesInfo[name]["Elements"] =                                     \
            std::to_string(attribute.m_Elements);                              \
                                                                               \
        if (attribute.m_IsSingleValue)                                         \
        {                                                                      \
            attributesInfo[name]["Value"] =                                    \
                ValueToString(attribute.m_DataSingleValue);                    \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            attributesInfo[name]["Value"] =                                    \
                "{ " + VectorToCSV(attribute.m_DataArray) + " }";              \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    } // end for
    return attributesInfo;
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

Engine &IO::Open(const std::string &name, const Mode mode,
                 MPI_Comm mpiComm_orig)
{
    if (m_DebugMode)
    {
        if (m_Engines.count(name) == 1)
        {
            throw std::invalid_argument("ERROR: IO Engine with name " + name +
                                        " already created, in call to Open.\n");
        }
    }

    MPI_Comm mpiComm;
    MPI_Comm_dup(mpiComm_orig, &mpiComm);
    std::shared_ptr<Engine> engine;
    const bool isDefaultEngine = m_EngineType.empty() ? true : false;
    std::string engineTypeLC = m_EngineType;
    if (!isDefaultEngine)
    {
        std::transform(engineTypeLC.begin(), engineTypeLC.end(),
                       engineTypeLC.begin(), ::tolower);
    }

    if (isDefaultEngine || engineTypeLC == "bpfile")
    {
        if (mode == Mode::Read)
            engine = std::make_shared<BPFileReader>(*this, name, mode, mpiComm);
        else
            engine = std::make_shared<BPFileWriter>(*this, name, mode, mpiComm);
    }
    else if (engineTypeLC == "hdfmixer")
    {
#ifdef ADIOS2_HAVE_HDF5
#if H5_VERSION_GE(1, 11, 0)
        if (mode == Mode::Read)
            engine = std::make_shared<HDF5ReaderP>(*this, name, mode, mpiComm);
        else
            engine = std::make_shared<HDFMixer>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: update HDF5 >= 1.11 to support VDS.");
#endif
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5 engine\n");
#endif
    }
    else if (engineTypeLC == "dataman")
    {
#ifdef ADIOS2_HAVE_DATAMAN
        if (mode == Mode::Read)
            engine =
                std::make_shared<DataManReader>(*this, name, mode, mpiComm);
        else
            engine =
                std::make_shared<DataManWriter>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't use DataMan engine\n");
#endif
    }
    else if (engineTypeLC == "sst")
    {
#ifdef ADIOS2_HAVE_SST
        if (mode == Mode::Read)
            engine = std::make_shared<SstReader>(*this, name, mode, mpiComm);
        else
            engine = std::make_shared<SstWriter>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "Sst library, can't use Sst engine\n");
#endif
    }
    else if (engineTypeLC == "adios1")
    {
#ifdef ADIOS2_HAVE_ADIOS1
        if (mode == Mode::Read)
            engine = std::make_shared<ADIOS1Reader>(*this, name, mode, mpiComm);
        else
            engine = std::make_shared<ADIOS1Writer>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with ADIOS "
            "1.x library, can't use ADIOS1 engine\n");
#endif
    }
    else if (engineTypeLC == "hdf5")
    {
#ifdef ADIOS2_HAVE_HDF5
        if (mode == Mode::Read)
            engine = std::make_shared<HDF5ReaderP>(*this, name, mode, mpiComm);
        else
            engine = std::make_shared<HDF5WriterP>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5 engine\n");
#endif
    }
    else if (engineTypeLC == "insitumpi")
    {
#ifdef ADIOS2_HAVE_MPI
        if (mode == Mode::Read)
            engine =
                std::make_shared<InSituMPIReader>(*this, name, mode, mpiComm);
        else
            engine =
                std::make_shared<InSituMPIWriter>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "MPI, can't use InSituMPI engine\n");
#endif
    }
    else if (engineTypeLC == "pluginengine")
    {
        engine = std::make_shared<PluginEngine>(*this, name, mode, mpiComm);
    }
    else if (engineTypeLC == "skeleton")
    {
        if (mode == Mode::Read)
            engine =
                std::make_shared<SkeletonReader>(*this, name, mode, mpiComm);
        else
            engine =
                std::make_shared<SkeletonWriter>(*this, name, mode, mpiComm);
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

Engine &IO::Open(const std::string &name, const Mode mode)
{
    return Open(name, mode, m_MPIComm);
}

void IO::FlushAll()
{
    for (auto &enginePair : m_Engines)
    {
        auto &engine = enginePair.second;
        if (engine->OpenMode() != Mode::Read)
        {
            enginePair.second->Flush();
        }
    }
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

} // end namespace adios2
