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

#include <memory>
#include <mutex>
#include <sstream>
#include <utility> // std::pair

#include "adios2/common/ADIOSMacros.h"

#include "adios2/engine/bp3/BP3Reader.h"
#include "adios2/engine/bp3/BP3Writer.h"
#include "adios2/engine/bp4/BP4Reader.h"
#include "adios2/engine/bp4/BP4Writer.h"
#ifdef ADIOS2_HAVE_BP5
#include "adios2/engine/bp5/BP5Reader.h"
#include "adios2/engine/bp5/BP5Writer.h"
#endif
#include "adios2/engine/inline/InlineReader.h"
#include "adios2/engine/inline/InlineWriter.h"
#include "adios2/engine/mhs/MhsReader.h"
#include "adios2/engine/mhs/MhsWriter.h"
#include "adios2/engine/null/NullEngine.h"
#include "adios2/engine/nullcore/NullCoreWriter.h"
#include "adios2/engine/skeleton/SkeletonReader.h"
#include "adios2/engine/skeleton/SkeletonWriter.h"

#include "adios2/helper/adiosComm.h"
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap
#include "adios2/helper/adiosString.h"
#include <adios2sys/SystemTools.hxx> // FileIsDirectory()

#ifdef ADIOS2_HAVE_DATAMAN // external dependencies
#include "adios2/engine/dataman/DataManReader.h"
#include "adios2/engine/dataman/DataManWriter.h"
#endif

#ifdef ADIOS2_HAVE_SST // external dependencies
#include "adios2/engine/sst/SstReader.h"
#include "adios2/engine/sst/SstWriter.h"
#endif

namespace adios2
{
namespace core
{

IO::EngineFactoryEntry IO_MakeEngine_HDF5();

namespace
{
std::unordered_map<std::string, IO::EngineFactoryEntry> Factory = {
    {"bp3",
     {IO::MakeEngine<engine::BP3Reader>, IO::MakeEngine<engine::BP3Writer>}},
    {"bp4",
     {IO::MakeEngine<engine::BP4Reader>, IO::MakeEngine<engine::BP4Writer>}},
    {"bp5",
#ifdef ADIOS2_HAVE_BP5
     {IO::MakeEngine<engine::BP5Reader>, IO::MakeEngine<engine::BP5Writer>}
#else
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "BP5 library, can't use BP5 engine\n")
#endif
    },
    {"dataman",
#ifdef ADIOS2_HAVE_DATAMAN
     {IO::MakeEngine<engine::DataManReader>,
      IO::MakeEngine<engine::DataManWriter>}
#else
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "DataMan library, can't use DataMan engine\n")
#endif
    },
    {"ssc", IO::NoEngineEntry("ERROR: this version didn't compile with "
                              "SSC library, can't use SSC engine\n")},
    {"mhs",
#ifdef ADIOS2_HAVE_MHS
     {IO::MakeEngine<engine::MhsReader>, IO::MakeEngine<engine::MhsWriter>}
#else
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "MHS library, can't use MHS engine\n")
#endif
    },
    {"sst",
#ifdef ADIOS2_HAVE_SST
     {IO::MakeEngine<engine::SstReader>, IO::MakeEngine<engine::SstWriter>}
#else
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "Sst library, can't use Sst engine\n")
#endif
    },
    {"effis",
#ifdef ADIOS2_HAVE_SST
     {IO::MakeEngine<engine::SstReader>, IO::MakeEngine<engine::SstWriter>}
#else
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "Sst library, can't use Sst engine\n")
#endif
    },
    {"dataspaces",
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "DataSpaces library, can't use DataSpaces engine\n")},
    {"hdf5",
#ifdef ADIOS2_HAVE_HDF5
     IO_MakeEngine_HDF5()
#else
     IO::NoEngineEntry("ERROR: this version didn't compile with "
                       "HDF5 library, can't use HDF5 engine\n")
#endif
    },
    {"insitumpi", IO::NoEngineEntry("ERROR: this version didn't compile with "
                                    "MPI, can't use InSituMPI engine\n")},
    {"skeleton",
     {IO::MakeEngine<engine::SkeletonReader>,
      IO::MakeEngine<engine::SkeletonWriter>}},
    {"inline",
     {IO::MakeEngine<engine::InlineReader>,
      IO::MakeEngine<engine::InlineWriter>}},
    {"null",
     {IO::MakeEngine<engine::NullEngine>, IO::MakeEngine<engine::NullEngine>}},
    {"nullcore",
     {IO::NoEngine("ERROR: nullcore engine does not support read mode"),
      IO::MakeEngine<engine::NullCoreWriter>}},
};

// Synchronize access to the factory in case one thread is
// looking up while another registers additional entries.
std::mutex FactoryMutex;

std::unordered_map<std::string, IO::EngineFactoryEntry>::const_iterator
FactoryLookup(std::string const &name)
{
    std::lock_guard<std::mutex> factoryGuard(FactoryMutex);
    return Factory.find(name);
}

struct ThrowError
{
    std::shared_ptr<Engine> operator()(IO &, const std::string &, const Mode,
                                       helper::Comm) const
    {
        throw std::invalid_argument(Err);
    }
    std::string Err;
};

} // end anonymous namespace

IO::MakeEngineFunc IO::NoEngine(std::string e) { return ThrowError{e}; }

IO::EngineFactoryEntry IO::NoEngineEntry(std::string e)
{
    return {NoEngine(e), NoEngine(e)};
}

void IO::RegisterEngine(const std::string &engineType, EngineFactoryEntry entry)
{
    std::lock_guard<std::mutex> factoryGuard(FactoryMutex);
    Factory[engineType] = std::move(entry);
}

IO::IO(ADIOS &adios, const std::string name, const bool inConfigFile,
       const std::string hostLanguage)
: m_ADIOS(adios), m_Name(name), m_HostLanguage(hostLanguage),
  m_InConfigFile(inConfigFile)
{
}

IO::~IO() = default;

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
        finalEngineType = "filestream";
        lf_InsertParam("OpenTimeoutSecs", "3600");
        lf_InsertParam("StreamReader", "true");
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
    PERFSTUBS_SCOPED_TIMER("IO::other");
    for (const auto &parameter : parameters)
    {
        m_Parameters[parameter.first] = parameter.second;
    }
}

void IO::SetParameters(const std::string &parameters)
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    adios2::Params parameterMap =
        adios2::helper::BuildParametersMap(parameters, '=', ',');
    SetParameters(parameterMap);
}

void IO::SetParameter(const std::string key, const std::string value) noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    m_Parameters[key] = value;
}

Params &IO::GetParameters() noexcept { return m_Parameters; }

void IO::ClearParameters() noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    m_Parameters.clear();
}

size_t IO::AddTransport(const std::string type, const Params &parameters)
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    Params parametersMap(parameters);

    if (parameters.count("transport") == 1 ||
        parameters.count("Transport") == 1)
    {
        throw std::invalid_argument("ERROR: key Transport (or transport) "
                                    "is not valid for transport type " +
                                    type + ", in call to AddTransport)");
    }

    CheckTransportType(type);

    parametersMap["transport"] = type;
    m_TransportsParameters.push_back(parametersMap);
    return m_TransportsParameters.size() - 1;
}

void IO::SetTransportParameter(const size_t transportIndex,
                               const std::string key, const std::string value)
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    if (transportIndex >= m_TransportsParameters.size())
    {
        throw std::invalid_argument(
            "ERROR: transportIndex is larger than "
            "transports created with AddTransport, for key: " +
            key + ", value: " + value + "in call to SetTransportParameter\n");
    }

    m_TransportsParameters[transportIndex][key] = value;
}

const VarMap &IO::GetVariables() const noexcept { return m_Variables; }

const AttrMap &IO::GetAttributes() const noexcept { return m_Attributes; }

bool IO::InConfigFile() const noexcept { return m_InConfigFile; }

void IO::SetDeclared() noexcept { m_IsDeclared = true; }

void IO::SetArrayOrder(const ArrayOrdering ArrayOrder) noexcept
{
    if (ArrayOrder == ArrayOrdering::Auto)
    {
        if (helper::IsRowMajor(m_HostLanguage))
            m_ArrayOrder = ArrayOrdering::RowMajor;
        else
            m_ArrayOrder = ArrayOrdering::ColumnMajor;
    }
    else
        m_ArrayOrder = ArrayOrder;
}

bool IO::IsDeclared() const noexcept { return m_IsDeclared; }

bool IO::RemoveVariable(const std::string &name) noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::RemoveVariable");
    bool isRemoved = false;
    auto itVariable = m_Variables.find(name);
    // variable exists
    if (itVariable != m_Variables.end())
    {
        m_Variables.erase(itVariable);
        isRemoved = true;
    }
    return isRemoved;
}

void IO::RemoveAllVariables() noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::RemoveAllVariables");
    m_Variables.clear();
}

bool IO::RemoveAttribute(const std::string &name) noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::RemoveAttribute");
    bool isRemoved = false;
    auto itAttribute = m_Attributes.find(name);
    // attribute exists
    if (itAttribute != m_Attributes.end())
    {
        // first remove the Attribute object
        const DataType type(itAttribute->second->m_Type);

        if (type == DataType::None)
        {
            // nothing to do
        }
        else
        {
            m_Attributes.erase(itAttribute);
            isRemoved = true;
        }
    }

    return isRemoved;
}

void IO::RemoveAllAttributes() noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::RemoveAllAttributes");
    m_Attributes.clear();
}

std::map<std::string, Params>
IO::GetAvailableVariables(const std::set<std::string> &keys) noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::GetAvailableVariables");

    std::map<std::string, Params> variablesInfo;
    for (const auto &variablePair : m_Variables)
    {
        const std::string variableName = variablePair.first;
        const DataType type = InquireVariableType(variableName);

        if (type == DataType::Compound)
        {
        }
#define declare_template_instantiation(T)                                      \
    else if (type == helper::GetDataType<T>())                                 \
    {                                                                          \
        variablesInfo[variableName] = GetVariableInfo<T>(variableName, keys);  \
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
    PERFSTUBS_SCOPED_TIMER("IO::GetAvailableAttributes");
    std::map<std::string, Params> attributesInfo;

    if (!variableName.empty())
    {
        auto itVariable = m_Variables.find(variableName);
        const DataType type = InquireVariableType(itVariable);

        if (type == DataType::Compound)
        {
        }
        else
        {
            attributesInfo = itVariable->second->GetAttributesInfo(
                *this, separator, fullNameKeys);
        }
        return attributesInfo;
    }

    // return all attributes if variable name is empty
    for (const auto &attributePair : m_Attributes)
    {
        const std::string &name = attributePair.first;

        if (attributePair.second->m_Type == DataType::Compound)
        {
        }
        else
        {
            attributesInfo[name] = attributePair.second->GetInfo();
        }
    }
    return attributesInfo;
}

DataType IO::InquireVariableType(const std::string &name) const noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    auto itVariable = m_Variables.find(name);
    return InquireVariableType(itVariable);
}

DataType IO::InquireVariableType(const VarMap::const_iterator itVariable) const
    noexcept
{
    if (itVariable == m_Variables.end())
    {
        return DataType::None;
    }

    const DataType type = itVariable->second->m_Type;

    if (m_ReadStreaming)
    {
        if (type == DataType::Compound)
        {
        }
        else
        {
            if (!itVariable->second->IsValidStep(m_EngineStep + 1))
            {
                return DataType::None;
            }
        }
    }

    return type;
}

DataType IO::InquireAttributeType(const std::string &name,
                                  const std::string &variableName,
                                  const std::string separator) const noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    const std::string globalName =
        helper::GlobalName(name, variableName, separator);

    auto itAttribute = m_Attributes.find(globalName);
    if (itAttribute == m_Attributes.end())
    {
        return DataType::None;
    }

    return itAttribute->second->m_Type;
}

void IO::AddOperation(const std::string &variable,
                      const std::string &operatorType,
                      const Params &parameters) noexcept
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    auto params = helper::LowerCaseParams(parameters);
    Operator *op = &m_ADIOS.DefineOperator(
        m_Name + "_" + variable + "_" + operatorType, operatorType, params);
    m_VarOpsPlaceholder[variable].emplace_back(Operation{op, params, Params()});
}

Engine &IO::Open(const std::string &name, const Mode mode, helper::Comm comm)
{
    PERFSTUBS_SCOPED_TIMER("IO::Open");
    auto itEngineFound = m_Engines.find(name);
    const bool isEngineFound = (itEngineFound != m_Engines.end());
    bool isEngineActive = false;
    Mode mode_to_use = mode;

    if (isEngineFound)
    {
        if (*itEngineFound->second)
        {
            isEngineActive = true;
        }
    }

    if (isEngineFound)
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
    /* BPFile for read needs to use BP5, BP4, or BP3 depending on the file's
     * version
     */
    if ((engineTypeLC == "file" || engineTypeLC == "bpfile" ||
         engineTypeLC == "bp" || isDefaultEngine))
    {
        if (helper::EndsWith(name, ".h5", false))
        {
            engineTypeLC = "hdf5";
        }
        else if ((mode_to_use == Mode::Read) ||
                 (mode_to_use == Mode::ReadRandomAccess))
        {
            if (adios2sys::SystemTools::FileIsDirectory(name))
            {
                char v = helper::BPVersion(name, comm, m_TransportsParameters);
                if (v == 'X')
                {
                    // BP4 did not create this file pre 2.8.0 so if not found,
                    // lets assume bp4
                    v = '4';
                }
                engineTypeLC = "bp";
                engineTypeLC.push_back(v);
            }
            else if (adios2sys::SystemTools::FileIsDirectory(name + ".tier0"))
            {
                engineTypeLC = "mhs";
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
            // File default for writing: BP4
            engineTypeLC = "bp4";
        }
    }

    // filestream is either BP5 or BP4 depending on .bpversion
    /* Note: Mismatch between BP4/BP5 writer and FileStream reader is not
       handled if writer has not created the directory yet, when FileStream
       falls back to default */
    if (engineTypeLC == "filestream")
    {
        char v = helper::BPVersion(name, comm, m_TransportsParameters);
        if (v == 'X')
        {
            // FileStream default: BP4
            v = '4';
        }
        engineTypeLC = "bp";
        engineTypeLC.push_back(v);
        // std::cout << "Engine " << engineTypeLC << " selected for FileStream"
        //          << std::endl;
    }

    if ((engineTypeLC != "bp5") && (mode_to_use == Mode::ReadRandomAccess))
    {
        // only BP5 special-cases file-reader random access mode
        mode_to_use = Mode::Read;
    }
    // For the inline engine, there must be exactly 1 reader, and exactly 1
    // writer.
    if (engineTypeLC == "inline")
    {
        if (mode_to_use == Mode::Append)
        {
            throw std::runtime_error(
                "Append mode is not supported for the inline engine.");
        }

        // See inline.rst:44
        if (mode_to_use == Mode::Sync)
        {
            throw std::runtime_error(
                "Sync mode is not supported for the inline engine.");
        }

        if (m_Engines.size() >= 2)
        {
            std::string msg =
                "Failed to add engine " + name + " to IO \'" + m_Name + "\'. ";
            msg += "An inline engine must have exactly one writer, and one "
                   "reader. ";
            msg += "There are already two engines declared, so no more can be "
                   "added.";
            throw std::runtime_error(msg);
        }
        // Now protect against declaration of two writers, or declaration of
        // two readers:
        if (m_Engines.size() == 1)
        {
            auto engine_ptr = m_Engines.begin()->second;
            if (engine_ptr->OpenMode() == mode_to_use)
            {
                std::string msg =
                    "The previously added engine " + engine_ptr->m_Name +
                    " is already opened in same mode requested for " + name +
                    ". ";
                msg += "The inline engine requires exactly one writer and one "
                       "reader.";
                throw std::runtime_error(msg);
            }
        }
    }

    auto f = FactoryLookup(engineTypeLC);
    if (f != Factory.end())
    {
        if ((mode_to_use == Mode::Read) ||
            (mode_to_use == Mode::ReadRandomAccess))
        {
            engine =
                f->second.MakeReader(*this, name, mode_to_use, std::move(comm));
        }
        else
        {
            engine =
                f->second.MakeWriter(*this, name, mode_to_use, std::move(comm));
        }
    }
    else
    {
        throw std::invalid_argument("ERROR: engine " + m_EngineType +
                                    " not supported, IO SetEngine must add "
                                    "a supported engine, in call to "
                                    "Open\n");
    }

    auto itEngine = m_Engines.emplace(name, std::move(engine));

    if (!itEngine.second)
    {
        throw std::invalid_argument("ERROR: engine of type " + m_EngineType +
                                    " and name " + name +
                                    " could not be created, in call to Open\n");
    }
    // return a reference
    return *itEngine.first->second.get();
}

Engine &IO::Open(const std::string &name, const Mode mode)
{
    return Open(name, mode, m_ADIOS.GetComm().Duplicate());
}
Group &IO::CreateGroup(char delimiter)
{
    m_Gr = std::make_shared<Group>("", delimiter, *this);
    m_Gr->BuildTree();
    return *m_Gr;
}
Engine &IO::GetEngine(const std::string &name)
{
    PERFSTUBS_SCOPED_TIMER("IO::other");
    auto itEngine = m_Engines.find(name);
    if (itEngine == m_Engines.end())
    {
        throw std::invalid_argument(
            "ERROR: engine name " + name +
            " could not be found, in call to GetEngine\n");
    }
    // return a reference
    return *itEngine->second.get();
}

void IO::RemoveEngine(const std::string &name)
{
    auto itEngine = m_Engines.find(name);
    if (itEngine != m_Engines.end())
    {
        m_Engines.erase(itEngine);
    }
}

void IO::FlushAll()
{
    PERFSTUBS_SCOPED_TIMER("IO::FlushAll");
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
    PERFSTUBS_SCOPED_TIMER("IO::other");
    for (auto itVariable = m_Variables.begin(); itVariable != m_Variables.end();
         ++itVariable)
    {
        const DataType type = InquireVariableType(itVariable);

        if (type == DataType::None)
        {
            continue;
        }

        if (type == DataType::Compound)
        {
        }
        else
        {
            VariableBase &variable = *itVariable->second;
            variable.CheckRandomAccessConflict(hint);
            variable.ResetStepsSelection(zeroStart);
            variable.m_RandomAccess = false;
        }
    }
}

void IO::SetPrefixedNames(const bool isStep) noexcept
{
    const std::set<std::string> attributes = helper::KeysToSet(m_Attributes);
    const std::set<std::string> variables = helper::KeysToSet(m_Variables);

    for (auto itVariable = m_Variables.begin(); itVariable != m_Variables.end();
         ++itVariable)
    {
        // if for each step (BP4), check if variable type is not empty
        // (means variable exist in that step)
        const DataType type = isStep ? InquireVariableType(itVariable)
                                     : itVariable->second->m_Type;

        if (type == DataType::None)
        {
            continue;
        }

        if (type == DataType::Compound)
        {
        }
        else
        {
            VariableBase &variable = *itVariable->second;
            variable.m_PrefixedVariables =
                helper::PrefixMatches(variable.m_Name, variables);
            variable.m_PrefixedAttributes =
                helper::PrefixMatches(variable.m_Name, attributes);
        }
    }

    m_IsPrefixedNames = true;
}

// PRIVATE
void IO::CheckAttributeCommon(const std::string &name) const
{
    auto itAttribute = m_Attributes.find(name);
    if (itAttribute != m_Attributes.end())
    {
        throw std::invalid_argument("ERROR: attribute " + name +
                                    " exists in IO object " + m_Name +
                                    ", in call to DefineAttribute\n");
    }
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
        const std::string, const bool);                                        \
    template Attribute<T> &IO::DefineAttribute<T>(                             \
        const std::string &, const T &, const std::string &,                   \
        const std::string, const bool);                                        \
    template Attribute<T> *IO::InquireAttribute<T>(                            \
        const std::string &, const std::string &, const std::string) noexcept;

ADIOS2_FOREACH_ATTRIBUTE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace core
} // end namespace adios2
