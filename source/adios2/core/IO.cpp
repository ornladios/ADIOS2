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
#include <utility> // std::pair

#include "adios2/common/ADIOSMacros.h"

#include "adios2/engine/bp3/BP3Reader.h"
#include "adios2/engine/bp3/BP3Writer.h"
#include "adios2/engine/bp4/BP4Reader.h"
#include "adios2/engine/bp4/BP4Writer.h"
#include "adios2/engine/inline/InlineReader.h"
#include "adios2/engine/inline/InlineWriter.h"
#include "adios2/engine/null/NullEngine.h"
#include "adios2/engine/nullcore/NullCoreWriter.h"
#include "adios2/engine/skeleton/SkeletonReader.h"
#include "adios2/engine/skeleton/SkeletonWriter.h"

#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap
#include "adios2/toolkit/profiling/taustubs/tautimer.hpp"
#include <adios2sys/SystemTools.hxx> // FileIsDirectory()

#ifdef ADIOS2_HAVE_DATAMAN // external dependencies
#include "adios2/engine/dataman/DataManReader.h"
#include "adios2/engine/dataman/DataManWriter.h"
#endif

#ifdef ADIOS2_HAVE_SSC // external dependencies
#include "adios2/engine/ssc/SscReader.h"
#include "adios2/engine/ssc/SscWriter.h"
#endif

#ifdef ADIOS2_HAVE_TABLE // external dependencies
#include "adios2/engine/table/TableWriter.h"
#endif

#ifdef ADIOS2_HAVE_SST // external dependencies
#include "adios2/engine/sst/SstReader.h"
#include "adios2/engine/sst/SstWriter.h"
#endif

#ifdef ADIOS2_HAVE_DATASPACES // external dependencies
#include "adios2/engine/dataspaces/DataSpacesReader.h"
#include "adios2/engine/dataspaces/DataSpacesWriter.h"
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

IO::IO(ADIOS &adios, const std::string name, const bool inConfigFile,
       const std::string hostLanguage, const bool debugMode)
: m_ADIOS(adios), m_Name(name), m_InConfigFile(inConfigFile),
  m_HostLanguage(hostLanguage), m_DebugMode(debugMode)
{
}

void IO::SetEngine(const std::string engineType) noexcept
{
    auto lf_InsertParam = [&](const std::string &key,
                              const std::string &value) {
        m_Parameters.insert(std::pair<std::string, std::string>(key, value));
    };

    /* First step in handling virtual engine names */
    std::string finalEngineType;
    std::string engineTypeLC = engineType;
    std::transform(engineTypeLC.begin(), engineTypeLC.end(),
                   engineTypeLC.begin(), ::tolower);
    if (engineTypeLC == "insituviz" || engineTypeLC == "insituvisualization")
    {
        finalEngineType = "SST";
        lf_InsertParam("FirstTimestepPrecious", "true");
        lf_InsertParam("RendezvousReaderCount", "0");
        lf_InsertParam("QueueLimit", "3");
        lf_InsertParam("QueueFullPolicy", "Discard");
        lf_InsertParam("AlwaysProvideLatestTimestep", "false");
    }
    else if (engineTypeLC == "insituanalysis")
    {
        finalEngineType = "SST";
        lf_InsertParam("FirstTimestepPrecious", "false");
        lf_InsertParam("RendezvousReaderCount", "1");
        lf_InsertParam("QueueLimit", "1");
        lf_InsertParam("QueueFullPolicy", "Block");
        lf_InsertParam("AlwaysProvideLatestTimestep", "false");
    }
    else if (engineTypeLC == "codecoupling")
    {
        finalEngineType = "SST";
        lf_InsertParam("FirstTimestepPrecious", "false");
        lf_InsertParam("RendezvousReaderCount", "1");
        lf_InsertParam("QueueLimit", "1");
        lf_InsertParam("QueueFullPolicy", "Block");
        lf_InsertParam("AlwaysProvideLatestTimestep", "false");
    }
    else if (engineTypeLC == "filestream")
    {
        finalEngineType = "BP4";
        lf_InsertParam("OpenTimeoutSecs", "3600");
    }
    /* "file" is handled entirely in IO::Open() as it needs the name */
    else
    {
        finalEngineType = engineType;
    }

    m_EngineType = finalEngineType;
}
void IO::SetIOMode(const IOMode ioMode) { m_IOMode = ioMode; }

void IO::SetParameters(const Params &parameters) noexcept
{
    TAU_SCOPED_TIMER("IO::other");
    for (const auto &parameter : parameters)
    {
        m_Parameters[parameter.first] = parameter.second;
    }
}

void IO::SetParameters(const std::string &parameters)
{
    TAU_SCOPED_TIMER("IO::other");
    adios2::Params parameterMap =
        adios2::helper::BuildParametersMap(parameters, '=', ',', false);
    SetParameters(parameterMap);
}

void IO::SetParameter(const std::string key, const std::string value) noexcept
{
    TAU_SCOPED_TIMER("IO::other");
    m_Parameters[key] = value;
}

Params &IO::GetParameters() noexcept { return m_Parameters; }

void IO::ClearParameters() noexcept
{
    TAU_SCOPED_TIMER("IO::other");
    m_Parameters.clear();
}

size_t IO::AddTransport(const std::string type, const Params &parameters)
{
    TAU_SCOPED_TIMER("IO::other");
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
    TAU_SCOPED_TIMER("IO::other");
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

bool IO::InConfigFile() const noexcept { return m_InConfigFile; }

void IO::SetDeclared() noexcept { m_IsDeclared = true; }

bool IO::IsDeclared() const noexcept { return m_IsDeclared; }

bool IO::RemoveVariable(const std::string &name) noexcept
{
    TAU_SCOPED_TIMER("IO::RemoveVariable");
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
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        auto &variableMap = GetVariableMap<T>();                               \
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
    TAU_SCOPED_TIMER("IO::RemoveAllVariables");
    m_Variables.clear();
#define declare_type(T) GetVariableMap<T>().clear();
    ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    m_Compound.clear();
}

bool IO::RemoveAttribute(const std::string &name) noexcept
{
    TAU_SCOPED_TIMER("IO::RemoveAttribute");
    bool isRemoved = false;
    auto itAttribute = m_Attributes.find(name);
    // attribute exists
    if (itAttribute != m_Attributes.end())
    {
        // first remove the Attribute object
        const std::string type(itAttribute->second.first);
        const unsigned int index(itAttribute->second.second);

        if (type.empty())
        {
            // nothing to do
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        auto &attributeMap = GetAttributeMap<T>();                             \
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
    TAU_SCOPED_TIMER("IO::RemoveAllAttributes");
    m_Attributes.clear();

#define declare_type(T) GetAttributeMap<T>().clear();
    ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_type)
#undef declare_type
}

std::map<std::string, Params> IO::GetAvailableVariables() noexcept
{
    TAU_SCOPED_TIMER("IO::GetAvailableVariables");
    std::map<std::string, Params> variablesInfo;
    for (const auto &variablePair : m_Variables)
    {
        const std::string name(variablePair.first);
        const std::string type = InquireVariableType(name);

        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        variablesInfo[name]["Type"] = type;                                    \
        Variable<T> &variable = *InquireVariable<T>(name);                     \
        variablesInfo[name]["AvailableStepsCount"] =                           \
            helper::ValueToString(variable.m_AvailableStepsCount);             \
        variablesInfo[name]["Shape"] = helper::VectorToCSV(variable.Shape());  \
        if (variable.m_SingleValue)                                            \
        {                                                                      \
            variablesInfo[name]["SingleValue"] = "true";                       \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            variablesInfo[name]["SingleValue"] = "false";                      \
            variablesInfo[name]["Min"] =                                       \
                helper::ValueToString(variable.Min());                         \
            variablesInfo[name]["Max"] =                                       \
                helper::ValueToString(variable.Max());                         \
        }                                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    return variablesInfo;
}

std::map<std::string, Params>
IO::GetAvailableAttributes(const std::string &variableName,
                           const std::string separator,
                           const bool fullNameKeys) noexcept
{
    TAU_SCOPED_TIMER("IO::GetAvailableAttributes");
    std::map<std::string, Params> attributesInfo;

    if (!variableName.empty())
    {
        auto itVariable = m_Variables.find(variableName);
        const std::string type = InquireVariableType(itVariable);

        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable =                                                \
            GetVariableMap<T>().at(itVariable->second.second);                 \
        attributesInfo =                                                       \
            variable.GetAttributesInfo(*this, separator, fullNameKeys);        \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

        return attributesInfo;
    }

    // return all attributes if variable name is empty
    for (const auto &attributePair : m_Attributes)
    {
        const std::string &name = attributePair.first;
        const std::string type = attributePair.second.first;

        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Attribute<T> &attribute =                                              \
            GetAttributeMap<T>().at(attributePair.second.second);              \
        attributesInfo[name] = attribute.GetInfo();                            \
    }
        ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }
    return attributesInfo;
}

std::string IO::InquireVariableType(const std::string &name) const noexcept
{
    TAU_SCOPED_TIMER("IO::other");
    auto itVariable = m_Variables.find(name);
    return InquireVariableType(itVariable);
}

std::string
IO::InquireVariableType(const DataMap::const_iterator itVariable) const noexcept
{
    if (itVariable == m_Variables.end())
    {
        return "";
    }

    const std::string type = itVariable->second.first;

    if (m_ReadStreaming)
    {
        if (type == "compound")
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        const Variable<T> &variable =                                          \
            const_cast<IO *>(this)->GetVariableMap<T>().at(                    \
                itVariable->second.second);                                    \
        if (!variable.IsValidStep(m_EngineStep + 1))                           \
        {                                                                      \
            return "";                                                         \
        }                                                                      \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation
    }

    return type;
}

std::string IO::InquireAttributeType(const std::string &name,
                                     const std::string &variableName,
                                     const std::string separator) const noexcept
{
    TAU_SCOPED_TIMER("IO::other");
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    auto itAttribute = m_Attributes.find(globalName);
    if (itAttribute == m_Attributes.end())
    {
        return "";
    }

    return itAttribute->second.first;
}

size_t IO::AddOperation(Operator &op, const Params &parameters) noexcept
{
    TAU_SCOPED_TIMER("IO::other");
    m_Operations.push_back(Operation{&op, parameters, Params()});
    return m_Operations.size() - 1;
}

Engine &IO::Open(const std::string &name, const Mode mode, helper::Comm comm)
{
    TAU_SCOPED_TIMER("IO::Open");
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

    std::shared_ptr<Engine> engine;
    const bool isDefaultEngine = m_EngineType.empty() ? true : false;
    std::string engineTypeLC = m_EngineType;
    if (!isDefaultEngine)
    {
        std::transform(engineTypeLC.begin(), engineTypeLC.end(),
                       engineTypeLC.begin(), ::tolower);
    }

    /* Second step in handling virtual engines */
    /* BPFile for read needs to use BP4 or BP3 depending on the file's version
     */
    if ((engineTypeLC == "file" || engineTypeLC == "bpfile" ||
         engineTypeLC == "bp" || isDefaultEngine))
    {
        if (helper::EndsWith(name, ".h5", false))
        {
            engineTypeLC = "hdf5";
        }
        else if (mode == Mode::Read)
        {
            if (adios2sys::SystemTools::FileIsDirectory(name))
            {
                engineTypeLC = "bp4";
            }
            else
            {
                if (helper::EndsWith(name, ".bp", false))
                {
                    engineTypeLC = "bp3";
                }
                else
                {
                    /* We need to figure out the type of file
                     * from the file itself
                     */
                    if (helper::IsHDF5File(name, comm, m_TransportsParameters))
                    {
                        engineTypeLC = "hdf5";
                    }
                    else
                    {
                        engineTypeLC = "bp3";
                    }
                }
            }
        }
        else
        {
            engineTypeLC = "bp4";
        }
    }

    if (engineTypeLC == "bp3")
    {
        if (mode == Mode::Read)
        {
            engine = std::make_shared<engine::BP3Reader>(*this, name, mode,
                                                         std::move(comm));
        }
        else
        {
            engine = std::make_shared<engine::BP3Writer>(*this, name, mode,
                                                         std::move(comm));
        }
    }
    else if (engineTypeLC == "bp4")
    {
        if (mode == Mode::Read)
        {
            engine = std::make_shared<engine::BP4Reader>(*this, name, mode,
                                                         std::move(comm));
        }
        else
        {
            engine = std::make_shared<engine::BP4Writer>(*this, name, mode,
                                                         std::move(comm));
        }
    }
    else if (engineTypeLC == "hdfmixer")
    {
#ifdef ADIOS2_HAVE_HDF5
#if H5_VERSION_GE(1, 11, 0)
        if (mode == Mode::Read)
            engine = std::make_shared<engine::HDF5ReaderP>(*this, name, mode,
                                                           std::move(comm));
        else
            engine = std::make_shared<engine::HDFMixer>(*this, name, mode,
                                                        std::move(comm));
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
                                                             std::move(comm));
        else
            engine = std::make_shared<engine::DataManWriter>(*this, name, mode,
                                                             std::move(comm));
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't use DataMan engine\n");
#endif
    }
    else if (engineTypeLC == "ssc")
    {
#ifdef ADIOS2_HAVE_SSC
        if (mode == Mode::Read)
            engine = std::make_shared<engine::SscReader>(*this, name, mode,
                                                         std::move(comm));
        else
            engine = std::make_shared<engine::SscWriter>(*this, name, mode,
                                                         std::move(comm));
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "SSC library, can't use SSC engine\n");
#endif
    }
    else if (engineTypeLC == "table")
    {
#ifdef ADIOS2_HAVE_TABLE
        if (mode == Mode::Write)
            engine = std::make_shared<engine::TableWriter>(*this, name, mode,
                                                           std::move(comm));
        else
            throw std::invalid_argument(
                "ERROR: Table engine only supports Write. It uses other "
                "engines as backend. Please use corresponding engines for "
                "Read\n");
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "Table library, can't use Table engine\n");
#endif
    }
    else if (engineTypeLC == "sst" || engineTypeLC == "effis")
    {
#ifdef ADIOS2_HAVE_SST
        if (mode == Mode::Read)
            engine = std::make_shared<engine::SstReader>(*this, name, mode,
                                                         std::move(comm));
        else
            engine = std::make_shared<engine::SstWriter>(*this, name, mode,
                                                         std::move(comm));
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "Sst library, can't use Sst engine\n");
#endif
    }
    else if (engineTypeLC == "dataspaces")
    {
#ifdef ADIOS2_HAVE_DATASPACES
        if (mode == Mode::Read)
            engine = std::make_shared<engine::DataSpacesReader>(
                *this, name, mode, std::move(comm));
        else
            engine = std::make_shared<engine::DataSpacesWriter>(
                *this, name, mode, std::move(comm));
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataSpaces library, can't use DataSpaces engine\n");
#endif
    }
    else if (engineTypeLC == "hdf5")
    {
#ifdef ADIOS2_HAVE_HDF5
        if (mode == Mode::Read)
            engine = std::make_shared<engine::HDF5ReaderP>(*this, name, mode,
                                                           std::move(comm));
        else
            engine = std::make_shared<engine::HDF5WriterP>(*this, name, mode,
                                                           std::move(comm));
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5 engine\n");
#endif
    }
    else if (engineTypeLC == "insitumpi")
    {
#ifdef ADIOS2_HAVE_MPI
        if (mode == Mode::Read)
            engine = std::make_shared<engine::InSituMPIReader>(
                *this, name, mode, std::move(comm));
        else
            engine = std::make_shared<engine::InSituMPIWriter>(
                *this, name, mode, std::move(comm));
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "MPI, can't use InSituMPI engine\n");
#endif
    }
    else if (engineTypeLC == "skeleton")
    {
        if (mode == Mode::Read)
            engine = std::make_shared<engine::SkeletonReader>(*this, name, mode,
                                                              std::move(comm));
        else
            engine = std::make_shared<engine::SkeletonWriter>(*this, name, mode,
                                                              std::move(comm));
    }
    else if (engineTypeLC == "inline")
    {
        if (mode == Mode::Read)
            engine = std::make_shared<engine::InlineReader>(*this, name, mode,
                                                            std::move(comm));
        else
            engine = std::make_shared<engine::InlineWriter>(*this, name, mode,
                                                            std::move(comm));
    }
    else if (engineTypeLC == "null")
    {
        engine = std::make_shared<engine::NullEngine>(*this, name, mode,
                                                      std::move(comm));
    }
    else if (engineTypeLC == "nullcore")
    {
        if (mode == Mode::Read)
            throw std::invalid_argument(
                "ERROR: nullcore engine does not support read mode");
        else
            engine = std::make_shared<engine::NullCoreWriter>(*this, name, mode,
                                                              std::move(comm));
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
    return Open(name, mode, m_ADIOS.GetComm().Duplicate());
}

Engine &IO::GetEngine(const std::string &name)
{
    TAU_SCOPED_TIMER("IO::other");
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
    TAU_SCOPED_TIMER("IO::FlushAll");
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
    TAU_SCOPED_TIMER("IO::other");
    for (auto itVariable = m_Variables.begin(); itVariable != m_Variables.end();
         ++itVariable)
    {
        const std::string &name = itVariable->first;
        const std::string type = InquireVariableType(itVariable);

        if (type.empty())
        {
            continue;
        }

        if (type == "compound")
        {
        }
// using relative start
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable =                                                \
            GetVariableMap<T>().at(itVariable->second.second);                 \
        variable.CheckRandomAccessConflict(hint);                              \
        variable.ResetStepsSelection(zeroStart);                               \
        variable.m_RandomAccess = false;                                       \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
}

void IO::SetPrefixedNames(const bool isStep) noexcept
{
    const std::set<std::string> attributes = helper::KeysToSet(m_Attributes);
    const std::set<std::string> variables = helper::KeysToSet(m_Variables);

    for (auto itVariable = m_Variables.begin(); itVariable != m_Variables.end();
         ++itVariable)
    {
        const std::string &name = itVariable->first;
        // if for each step (BP4), check if variable type is not empty (means
        // variable exist in that step)
        const std::string type =
            isStep ? InquireVariableType(itVariable) : itVariable->second.first;

        if (type.empty())
        {
            continue;
        }

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        Variable<T> &variable =                                                \
            GetVariableMap<T>().at(itVariable->second.second);                 \
        variable.m_PrefixedVariables =                                         \
            helper::PrefixMatches(variable.m_Name, variables);                 \
        variable.m_PrefixedAttributes =                                        \
            helper::PrefixMatches(variable.m_Name, attributes);                \
    }
        ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type
    }

    m_IsPrefixedNames = true;
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
