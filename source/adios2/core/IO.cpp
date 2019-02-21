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

#include "adios2/engine/bp3/BP3Reader.h"
#include "adios2/engine/bp3/BP3Writer.h"
#include "adios2/engine/inline/InlineReader.h"
#include "adios2/engine/inline/InlineWriter.h"

/*BP4 engine headers*/
#include "adios2/engine/bp4/BP4Reader.h"
#include "adios2/engine/bp4/BP4Writer.h"

#include "adios2/engine/skeleton/SkeletonReader.h"
#include "adios2/engine/skeleton/SkeletonWriter.h"
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap

#ifdef ADIOS2_HAVE_DATAMAN // external dependencies
#include "adios2/engine/dataman/DataManReader.h"
#include "adios2/engine/dataman/DataManWriter.h"
#endif

#ifdef ADIOS2_HAVE_WDM // external dependencies
#include "adios2/engine/wdm/WdmReader.h"
#include "adios2/engine/wdm/WdmWriter.h"
#endif

#ifdef ADIOS2_HAVE_SST // external dependencies
#include "adios2/engine/sst/SstReader.h"
#include "adios2/engine/sst/SstWriter.h"
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
namespace core
{

IO::IO(ADIOS &adios, const std::string name, MPI_Comm mpiComm,
       const bool inConfigFile, const std::string hostLanguage,
       const bool debugMode)
: m_ADIOS(adios), m_Name(name), m_MPIComm(mpiComm),
  m_InConfigFile(inConfigFile), m_HostLanguage(hostLanguage),
  m_DebugMode(debugMode)
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

size_t IO::AddTransport(const std::string type, const Params &parameters)
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
    return m_TransportsParameters.size() - 1;
}

void IO::SetTransportParameter(const size_t transportIndex,
                               const std::string key, const std::string value)
{
    if (m_DebugMode)
    {
        if (transportIndex >= m_TransportsParameters.size())
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
        const DataType type(itVariable->second.m_Type);
        const unsigned int index(itVariable->second.m_Index);

        if (type == DataType::Compound)
        {
            auto variableMap = m_Compound;
            variableMap.erase(index);
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        auto variableMap = GetVariableMap<T>();                                \
        variableMap.erase(index);                                              \
        isRemoved = true;                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
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
        // first remove the Attribute object
        const DataType type(itAttribute->second.m_Type);
        const unsigned int index(itAttribute->second.m_Index);

        if (type.empty())
        {
            // nothing to do
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        auto attributeMap = GetAttributeMap<T>();                              \
        attributeMap.erase(index);                                             \
        isRemoved = true;                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
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
    ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
}

std::map<std::string, Params> IO::GetAvailableVariables() noexcept
{
    std::map<std::string, Params> variablesInfo;
    for (const auto &variablePair : m_Variables)
    {
        const std::string name(variablePair.first);
        const DataType type = InquireVariableType(name);

        if (type == DataType::Compound)
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        variablesInfo[name]["Type"] = type.ToString();                         \
        Variable<T> &variable = *InquireVariable<T>(name);                     \
        variablesInfo[name]["AvailableStepsCount"] =                           \
            helper::ValueToString(variable.m_AvailableStepsCount);             \
        variablesInfo[name]["Shape"] = helper::VectorToCSV(variable.m_Shape);  \
        if (variable.m_SingleValue)                                            \
        {                                                                      \
            variablesInfo[name]["SingleValue"] = "true";                       \
            variablesInfo[name]["Value"] =                                     \
                helper::ValueToString(variable.m_Value);                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            variablesInfo[name]["SingleValue"] = "false";                      \
            variablesInfo[name]["Min"] =                                       \
                helper::ValueToString(variable.m_Min);                         \
            variablesInfo[name]["Max"] =                                       \
                helper::ValueToString(variable.m_Max);                         \
        }                                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    return variablesInfo;
}

std::map<std::string, Params>
IO::GetAvailableAttributes(const std::string &variableName,
                           const std::string separator) noexcept
{
    std::map<std::string, Params> attributesInfo;
    const std::string variablePrefix = variableName + separator;

    for (const auto &attributePair : m_Attributes)
    {
        const std::string absoluteName(attributePair.first);
        std::string name = absoluteName;
        if (!variableName.empty())
        {
            // valid associated attribute
            if (absoluteName.size() <= variablePrefix.size())
            {
                continue;
            }

            if (absoluteName.compare(0, variablePrefix.size(),
                                     variablePrefix) == 0)
            {
                name = absoluteName.substr(variablePrefix.size());
            }
            else
            {
                continue;
            }
        }

        const DataType type(attributePair.second.m_Type);
        attributesInfo[name]["Type"] = type.ToString();

        if (type == DataType::Compound)
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        Attribute<T> &attribute = *InquireAttribute<T>(absoluteName);          \
        attributesInfo[name]["Elements"] =                                     \
            std::to_string(attribute.m_Elements);                              \
                                                                               \
        if (attribute.m_IsSingleValue)                                         \
        {                                                                      \
            attributesInfo[name]["Value"] =                                    \
                helper::ValueToString(attribute.m_DataSingleValue);            \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            attributesInfo[name]["Value"] =                                    \
                "{ " + helper::VectorToCSV(attribute.m_DataArray) + " }";      \
        }                                                                      \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

    } // end for
    return attributesInfo;
}

DataType IO::InquireVariableType(const std::string &name) const noexcept
{
    auto itVariable = m_Variables.find(name);
    if (itVariable == m_Variables.end())
    {
        return DataType();
    }

    const DataType type = itVariable->second.m_Type;

    if (m_ReadStreaming)
    {
        if (type == DataType::Compound)
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        const Variable<T> &variable =                                          \
            const_cast<IO *>(this)->GetVariableMap<T>().at(                    \
                itVariable->second.m_Index);                                   \
        if (!variable.IsValidStep(m_EngineStep + 1))                           \
        {                                                                      \
            return DataType();                                                 \
        }                                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    return type;
}

DataType IO::InquireAttributeType(const std::string &name,
                                  const std::string &variableName,
                                  const std::string separator) const noexcept
{
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    auto itAttribute = m_Attributes.find(globalName);
    if (itAttribute == m_Attributes.end())
    {
        return DataType();
    }

    return itAttribute->second.m_Type;
}

size_t IO::AddOperation(Operator &op, const Params &parameters) noexcept
{
    m_Operations.push_back(Operation{&op, parameters, Params()});
    return m_Operations.size() - 1;
}

Engine &IO::Open(const std::string &name, const Mode mode,
                 MPI_Comm mpiComm_orig)
{
    auto itEngineFound = m_Engines.find(name);
    const bool isEngineFound = (itEngineFound != m_Engines.end());
    bool isEngineActive = false;
    if (isEngineFound)
    {
        if (*itEngineFound->second)
        {
            isEngineActive = true;
        }
    }

    if (m_DebugMode && isEngineFound)
    {
        if (isEngineActive) // check if active
        {
            throw std::invalid_argument("ERROR: IO Engine with name " + name +
                                        " already created and is active (Close "
                                        "not called yet), in call to Open.\n");
        }
    }

    if (isEngineFound)
    {
        if (!isEngineActive)
        {
            m_Engines.erase(name);
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

    if (isDefaultEngine || engineTypeLC == "bpfile" || engineTypeLC == "bp3" ||
        engineTypeLC == "bp")
    {
        if (mode == Mode::Read)
        {
            engine =
                std::make_shared<engine::BP3Reader>(*this, name, mode, mpiComm);
        }
        else
        {
            engine =
                std::make_shared<engine::BP3Writer>(*this, name, mode, mpiComm);
        }

        m_EngineType = "bp";
    }
    else if (engineTypeLC == "bp4" || engineTypeLC == "bp4file")
    {
        if (mode == Mode::Read)
        {
            engine =
                std::make_shared<engine::BP4Reader>(*this, name, mode, mpiComm);
        }
        else
        {
            engine =
                std::make_shared<engine::BP4Writer>(*this, name, mode, mpiComm);
        }

        m_EngineType = "bp4file";
    }
    else if (engineTypeLC == "hdfmixer")
    {
#ifdef ADIOS2_HAVE_HDF5
#if H5_VERSION_GE(1, 11, 0)
        if (mode == Mode::Read)
            engine = std::make_shared<engine::HDF5ReaderP>(*this, name, mode,
                                                           mpiComm);
        else
            engine =
                std::make_shared<engine::HDFMixer>(*this, name, mode, mpiComm);
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
            engine = std::make_shared<engine::DataManReader>(*this, name, mode,
                                                             mpiComm);
        else
            engine = std::make_shared<engine::DataManWriter>(*this, name, mode,
                                                             mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't use DataMan engine\n");
#endif
    }
    else if (engineTypeLC == "wdm")
    {
#ifdef ADIOS2_HAVE_WDM
        if (mode == Mode::Read)
            engine =
                std::make_shared<engine::WdmReader>(*this, name, mode, mpiComm);
        else
            engine =
                std::make_shared<engine::WdmWriter>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't use DataMan engine\n");
#endif
    }
    else if (engineTypeLC == "sst" || engineTypeLC == "effis")
    {
#ifdef ADIOS2_HAVE_SST
        if (mode == Mode::Read)
            engine =
                std::make_shared<engine::SstReader>(*this, name, mode, mpiComm);
        else
            engine =
                std::make_shared<engine::SstWriter>(*this, name, mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "Sst library, can't use Sst engine\n");
#endif
    }
    else if (engineTypeLC == "hdf5")
    {
#ifdef ADIOS2_HAVE_HDF5
        if (mode == Mode::Read)
            engine = std::make_shared<engine::HDF5ReaderP>(*this, name, mode,
                                                           mpiComm);
        else
            engine = std::make_shared<engine::HDF5WriterP>(*this, name, mode,
                                                           mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5 engine\n");
#endif
    }
    else if (engineTypeLC == "insitumpi")
    {
#ifdef ADIOS2_HAVE_MPI
        if (mode == Mode::Read)
            engine = std::make_shared<engine::InSituMPIReader>(*this, name,
                                                               mode, mpiComm);
        else
            engine = std::make_shared<engine::InSituMPIWriter>(*this, name,
                                                               mode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "MPI, can't use InSituMPI engine\n");
#endif
    }
    else if (engineTypeLC == "skeleton")
    {
        if (mode == Mode::Read)
            engine = std::make_shared<engine::SkeletonReader>(*this, name, mode,
                                                              mpiComm);
        else
            engine = std::make_shared<engine::SkeletonWriter>(*this, name, mode,
                                                              mpiComm);
    }
    else if (engineTypeLC == "inline")
    {
        if (mode == Mode::Read)
            engine = std::make_shared<engine::InlineReader>(*this, name, mode,
                                                            mpiComm);
        else
            engine = std::make_shared<engine::InlineWriter>(*this, name, mode,
                                                            mpiComm);
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

Engine &IO::GetEngine(const std::string &name)
{
    auto itEngine = m_Engines.find(name);
    if (m_DebugMode)
    {
        if (itEngine == m_Engines.end())
        {
            throw std::invalid_argument(
                "ERROR: engine name " + name +
                " could not be found, in call to GetEngine\n");
        }
    }
    // return a reference
    return *itEngine->second.get();
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

void IO::ResetVariablesStepSelection(const bool zeroStart,
                                     const std::string hint)
{
    const auto &variablesData = GetVariablesDataMap();

    for (const auto &variableData : variablesData)
    {
        const std::string name = variableData.first;
        const DataType type = InquireVariableType(name);

        if (type.empty())
        {
            continue;
        }

        if (type == DataType::Compound)
        {
        }
// using relative start
#define declare_type(T)                                                        \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        Variable<T> *variable = InquireVariable<T>(name);                      \
        variable->CheckRandomAccessConflict(hint);                             \
        variable->ResetStepsSelection(zeroStart);                              \
        variable->m_RandomAccess = false;                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void IO::LockDefinitions() noexcept { m_DefinitionsLocked = true; };

// PRIVATE
int IO::GetMapIndex(const std::string &name, const DataMap &dataMap) const
    noexcept
{
    auto itName = dataMap.find(name);
    if (itName == dataMap.end())
    {
        return -1;
    }
    return itName->second.m_Index;
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
    template Variable<T> &IO::DefineVariable<T>(const std::string &,           \
                                                const Dims &, const Dims &,    \
                                                const Dims &, const bool);     \
    template Variable<T> *IO::InquireVariable<T>(const std::string &) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(define_template_instantiation)
#undef define_template_instatiation

#define declare_template_instantiation(T)                                      \
    template Attribute<T> &IO::DefineAttribute<T>(                             \
        const std::string &, const T *, const size_t, const std::string &,     \
        const std::string);                                                    \
    template Attribute<T> &IO::DefineAttribute<T>(                             \
        const std::string &, const T &, const std::string &,                   \
        const std::string);                                                    \
    template Attribute<T> *IO::InquireAttribute<T>(                            \
        const std::string &, const std::string &, const std::string) noexcept;

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
