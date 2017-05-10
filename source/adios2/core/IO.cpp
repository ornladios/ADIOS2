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
#include "adios2/helper/adiosFunctions.h" //BuildParametersMap

#include "adios2/engine/bp/BPFileWriter.h"

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
#endif

namespace adios
{

IO::IO(const std::string name, MPI_Comm mpiComm, const bool inConfigFile,
       const bool debugMode)
: m_Name(name), m_MPIComm(mpiComm), m_InConfigFile(inConfigFile),
  m_DebugMode(debugMode)
{
}

void IO::SetEngine(const std::string engineType) { m_EngineType = engineType; }
void IO::SetIOMode(const IOMode ioMode) { m_IOMode = ioMode; };

VariableCompound &IO::GetVariableCompound(const std::string &name)
{
    return m_Compound.at(GetVariableIndex(name));
}

std::string IO::GetVariableType(const std::string &name)
{
    std::string type;

    auto itVariable = m_Variables.find(name);
    if (itVariable != m_Variables.end())
    {
        type = itVariable->second.first;
    }

    return type;
}

bool IO::InConfigFile() { return m_InConfigFile; };

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

    if (isRemoved == true)
    {
        m_Variables.erase(name);
    }

    return isRemoved;
}

std::shared_ptr<Engine> IO::Open(const std::string &name,
                                 const OpenMode openMode, MPI_Comm mpiComm)
{
    if (m_DebugMode == true)
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
                (openMode == OpenMode::Write || openMode == OpenMode::w ||
                 openMode == OpenMode::Append || openMode == OpenMode::a)
            ? true
            : false;

    const bool isDefaultReader =
        m_EngineType.empty() &&
                (openMode == OpenMode::Read || openMode == OpenMode::r ||
                 openMode == OpenMode::ReadWrite || openMode == OpenMode::rw)
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
    return engine;
}

std::shared_ptr<Engine> IO::Open(const std::string &name,
                                 const OpenMode openMode)
{
    return Open(name, openMode, m_MPIComm);
}

// PRIVATE Functions
unsigned int
IO::AddTransportParameters(const std::string type,
                           const std::vector<std::string> &parameters)
{
    if (m_DebugMode == true)
    {
        if (type.empty() || type.find("=") != type.npos)
        {
            throw std::invalid_argument(
                "ERROR: first argument in AddTransport must "
                "be a single word for transport\n");
        }
    }

    Params mapParameters = BuildParametersMap(parameters, m_DebugMode);

    if (m_DebugMode == true)
    {
        if (mapParameters.count("transport") == 1)
        {
            std::invalid_argument("ERROR: transport can't be redefined with "
                                  "\"transport=type\", "
                                  "type must be the first argument\n");
        }
    }

    mapParameters["transport"] = type;
    m_TransportsParameters.push_back(std::move(mapParameters));
    return static_cast<unsigned int>(m_TransportsParameters.size() - 1);
}

unsigned int IO::GetVariableIndex(const std::string &name)
{
    if (m_DebugMode == true)
    {
        if (VariableExists(name) == false)
        {
            throw std::invalid_argument(
                "ERROR: variable " + m_Name +
                " wasn't created with DefineVariable, in call to IO object " +
                m_Name + " GetVariable\n");
        }
    }
    auto itVariable = m_Variables.find(name);
    return itVariable->second.second;
}

bool IO::VariableExists(const std::string &name)
{
    bool exists = false;
    if (m_Variables.count(name) == 1)
    {
        exists = true;
    }
    return exists;
}

// Explicitly instantiate the necessary public template implementations
#define define_template_instantiation(T)                                       \
    template Variable<T> &IO::DefineVariable<T>(                               \
        const std::string &, const Dims, const Dims, const Dims, const bool);  \
    template Variable<T> &IO::GetVariable<T>(const std::string &);

ADIOS2_FOREACH_TYPE_1ARG(define_template_instantiation)
#undef define_template_instatiation

} // end namespace adios
