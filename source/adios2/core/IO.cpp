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

#include "adios2/ADIOSMPI.h"
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

#ifdef ADIOS2_HAVE_PYTHON
#include "adios2/engine/pyengine/PythonEngine.h"
#endif

namespace adios2
{

IO::IO(const std::string name, MPI_Comm mpiComm, const bool inConfigFile,
       const bool debugMode)
: m_Name(name), m_MPIComm(mpiComm), m_InConfigFile(inConfigFile),
  m_DebugMode(debugMode)
{
}

void IO::SetEngine(const std::string engineType) { m_EngineType = engineType; }
const std::string IO::GetEngine() const { return m_EngineType; }

void IO::SetIOMode(const IOMode ioMode) { m_IOMode = ioMode; };
const IOMode IO::GetIOMode() const { return m_IOMode; }

void IO::SetParameters(const Params &parameters) { m_Parameters = parameters; }

void IO::SetSingleParameter(const std::string key,
                            const std::string value) noexcept
{
    m_Parameters[key] = value;
}

Params &IO::GetParameters() { return m_Parameters; }

const Params &IO::GetParameters() const { return m_Parameters; }

unsigned int IO::AddTransport(const std::string type, const Params &parameters)
{
    Params parametersMap(parameters);
    if (m_DebugMode)
    {
        CheckTransportType(type);
    }

    parametersMap["transport"] = type;
    m_TransportsParameters.push_back(parametersMap);
    return static_cast<unsigned int>(m_TransportsParameters.size() - 1);
}

const std::vector<Params> &IO::GetTransportParameters() const
{
    return m_TransportsParameters;
}

void IO::SetTransportSingleParameter(const unsigned int transportIndex,
                                     const std::string key,
                                     const std::string value)
{
    if (m_DebugMode)
    {
        if (transportIndex >=
            static_cast<unsigned int>(m_TransportsParameters.size()))
        {
            throw std::invalid_argument("ERROR: transportIndex is larger than "
                                        "transports created with AddTransport "
                                        "function calls\n");
        }
    }

    m_TransportsParameters[transportIndex][key] = value;
}

VariableCompound &
IO::DefineVariableCompound(const std::string &name, const size_t sizeOfVariable,
                           const Dims &shape, const Dims &start,
                           const Dims &count, const bool constantDims)
{
    if (m_DebugMode)
    {
        auto itVariable = m_Variables.find(name);
        if (!IsEnd(itVariable, m_Variables))
        {
            throw std::invalid_argument("ERROR: variable " + name +
                                        " exists in IO object " + m_Name +
                                        ", in call to DefineVariable\n");
        }
    }
    const unsigned int size = m_Compound.size();
    auto itVariableCompound = m_Compound.emplace(
        size, VariableCompound(name, sizeOfVariable, shape, start, count,
                               constantDims, m_DebugMode));
    m_Variables.emplace(name, std::make_pair("compound", size));
    return itVariableCompound.first->second;
}

VariableCompound &IO::GetVariableCompound(const std::string &name)
{
    return m_Compound.at(GetMapIndex(name, m_Variables, "VariableCompound"));
}

const DataMap &IO::GetAttributesDataMap() const noexcept
{
    return m_Attributes;
}

VariableBase *IO::GetVariableBase(const std::string &name) noexcept
{
    VariableBase *variableBase = nullptr;
    auto itVariable = m_Variables.find(name);
    if (itVariable == m_Variables.end())
    {
        return variableBase;
    }

    const std::string type(itVariable->second.first);
    if (type == "compound")
    {
        variableBase = &GetVariableCompound(name);
    }
#define declare_type(T)                                                        \
    else if (type == GetType<T>()) { variableBase = &GetVariable<T>(name); }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    return variableBase;
}

std::string IO::GetVariableType(const std::string &name) const
{
    std::string type;

    auto itVariable = m_Variables.find(name);
    if (itVariable != m_Variables.end())
    {
        type = itVariable->second.first;
    }

    return type;
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

std::shared_ptr<Engine> IO::Open(const std::string &name,
                                 const OpenMode openMode, MPI_Comm mpiComm)
{
    if (m_DebugMode)
    {
        // Check if Engine already exists
        if (m_EngineNames.count(name) == 1)
        {
            throw std::invalid_argument(
                "ERROR: IO Engine with name " + name +
                " already created by Open, in call from Open.\n");
        }
    }

    std::shared_ptr<Engine> engine;
    m_EngineNames.insert(name);

    const bool isDefaultWriter =
        m_EngineType.empty() &&
                (openMode == OpenMode::Write || openMode == OpenMode::Append)
            ? true
            : false;

    const bool isDefaultReader =
        m_EngineType.empty() &&
                (openMode == OpenMode::Read || openMode == OpenMode::ReadWrite)
            ? true
            : false;

    if (isDefaultWriter || m_EngineType == "BPFileWriter")
    {
        engine = std::make_shared<BPFileWriter>(*this, name, openMode, mpiComm);
    }
    else if (isDefaultReader || m_EngineType == "BPFileReader")
    {
        // engine = std::make_shared<BPFileReader>(*this, name, openMode,
        // mpiComm);
    }
    else if (m_EngineType == "HDFMixer")
    {
#ifdef ADIOS2_HAVE_HDF5
#if H5_VERSION_GE(1, 11, 0)
        engine = std::make_shared<HDFMixer>(*this, name, openMode, mpiComm);
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
            std::make_shared<DataManWriter>(*this, name, openMode, mpiComm);
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
            std::make_shared<DataManReader>(*this, name, openMode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "DataMan library, can't Open DataManReader\n");
#endif
    }
    else if (m_EngineType == "ADIOS1Writer")
    {
#ifdef ADIOS2_HAVE_ADIOS1
        engine = std::make_shared<ADIOS1Writer>(*this, name, openMode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with ADIOS "
            "1.x library, can't Open ADIOS1Writer\n");
#endif
    }
    else if (m_EngineType == "ADIOS1Reader")
    {
#ifdef ADIOS2_HAVE_ADIOS1
        engine = std::make_shared<ADIOS1Reader>(*this, name, openMode, mpiComm);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with ADIOS "
            "1.x library, can't Open ADIOS1Reader\n");
#endif
    }
    else if (m_EngineType == "HDF5Writer")
    {
#ifdef ADIOS2_HAVE_HDF5
        engine = std::make_shared<HDF5WriterP>(*this, name, openMode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else if (m_EngineType == "HDF5Reader")
    {
#ifdef ADIOS2_HAVE_HDF5
        engine = std::make_shared<HDF5ReaderP>(*this, name, openMode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else if (m_EngineType == "PluginEngine")
    {
        engine = std::make_shared<PluginEngine>(*this, name, openMode, mpiComm);
    }
    else if (m_EngineType == "PythonEngine")
    {
#ifdef ADIOS2_HAVE_PYTHON
        engine = std::make_shared<PythonEngine>(*this, name, openMode, mpiComm);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "Python enabled, can't use PythonEngine\n");
#endif
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

    return engine;
}

std::shared_ptr<Engine> IO::Open(const std::string &name,
                                 const OpenMode openMode)
{
    return Open(name, openMode, m_MPIComm);
}

// PRIVATE Functions
unsigned int IO::GetMapIndex(const std::string &name, const DataMap &dataMap,
                             const std::string hint) const
{
    auto itDataMap = dataMap.find(name);

    if (m_DebugMode)
    {
        if (IsEnd(itDataMap, dataMap))
        {
            throw std::invalid_argument("ERROR: " + hint + " " + m_Name +
                                        " wasn't created with Define " + hint +
                                        ", in call to IO object " + m_Name +
                                        " Get" + hint + "\n");
        }
    }
    return itDataMap->second.second;
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
    template Variable<T> &IO::GetVariable<T>(const std::string &);

ADIOS2_FOREACH_TYPE_1ARG(define_template_instantiation)
#undef define_template_instatiation

#define declare_template_instantiation(T)                                      \
    template Attribute<T> &IO::DefineAttribute<T>(const std::string &,         \
                                                  const T *, const size_t);    \
    template Attribute<T> &IO::DefineAttribute<T>(const std::string &,         \
                                                  const T &);                  \
    template Attribute<T> &IO::GetAttribute(const std::string &);

ADIOS2_FOREACH_ATTRIBUTE_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace adios
