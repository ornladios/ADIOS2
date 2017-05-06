/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy
 */

#include "ADIOS.h"
#include "ADIOS.tcc"

#include <fstream>
#include <ios> //std::ios_base::failure
#include <iostream>
#include <sstream>
#include <utility>

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/core/adiosFunctions.h"
#include "adios2/engine/bp/BPFileReader.h"
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

ADIOS::ADIOS(const Verbose verbose, const bool debugMode)
: ADIOS("", MPI_COMM_SELF, verbose, debugMode)
{
}

ADIOS::ADIOS(const std::string config, const Verbose verbose,
             const bool debugMode)
: ADIOS(config, MPI_COMM_SELF, verbose, debugMode)
{
}

ADIOS::ADIOS(const std::string configFile, MPI_Comm mpiComm,
             const Verbose verbose, const bool debugMode)
: m_MPIComm(mpiComm), m_ConfigFile(configFile), m_DebugMode(debugMode)
{
    InitMPI();
    // InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage,
    // m_Transforms, m_Groups );
}

ADIOS::ADIOS(MPI_Comm mpiComm, const Verbose verbose, const bool debugMode)
: ADIOS("", mpiComm, verbose, debugMode)
{
}

// ADIOS::~ADIOS() {}

void ADIOS::InitMPI()
{
    if (m_DebugMode == true)
    {
        if (m_MPIComm == MPI_COMM_NULL)
        {
            throw std::ios_base::failure(
                "ERROR: engine communicator is MPI_COMM_NULL,"
                " in call to ADIOS Open or Constructor\n");
        }
    }

    MPI_Comm_rank(m_MPIComm, &m_RankMPI);
    MPI_Comm_size(m_MPIComm, &m_SizeMPI);
}

Method &ADIOS::DeclareMethod(const std::string methodName)
{
    if (m_DebugMode == true)
    {
        if (m_Methods.count(methodName) == 1)
        {
            throw std::invalid_argument(
                "ERROR: method " + methodName +
                " already declared, from DeclareMethod\n");
        }
    }
    m_Methods.emplace(methodName, Method(methodName, m_DebugMode));
    return m_Methods.at(methodName);
}

std::shared_ptr<Engine> ADIOS::Open(const std::string &name,
                                    const std::string accessMode,
                                    MPI_Comm mpiComm, const Method &method)
{
    if (m_DebugMode == true)
    {
        if (m_EngineNames.count(name) == 1) // Check if Engine already exists
        {
            throw std::invalid_argument(
                "ERROR: engine name " + name +
                " already created by Open, in call from Open.\n");
        }
    }

    m_EngineNames.insert(name);

    const std::string type(method.m_Type);

    const bool isDefaultWriter =
        (accessMode == "w" || accessMode == "write" || accessMode == "a" ||
         accessMode == "append") &&
                type.empty()
            ? true
            : false;

    const bool isDefaultReader =
        (accessMode == "r" || accessMode == "read") && type.empty() ? true
                                                                    : false;

    if (isDefaultWriter || type == "BPFileWriter" || type == "bpfilewriter")
    {
        return std::make_shared<BPFileWriter>(*this, name, accessMode, mpiComm,
                                              method);
    }
    else if (isDefaultReader || type == "BPReader" || type == "bpreader")
    {
        return std::make_shared<BPFileReader>(*this, name, accessMode, mpiComm,
                                              method);
    }
    else if (type == "SIRIUS" || type == "sirius" || type == "Sirius")
    {
        // not yet supported
        // return std::make_shared<engine::DataMan>( *this, name, accessMode,
        // mpiComm, method, iomode, timeout_sec, m_DebugMode, method.m_nThreads
        // );
    }
    else if (type == "DataManWriter")
    {
#ifdef ADIOS2_HAVE_DATAMAN
        return std::make_shared<DataManWriter>(*this, name, accessMode, mpiComm,
                                               method);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "Dataman library, can't Open DataManWriter\n");
#endif
    }
    else if (type == "DataManReader")
    {
#ifdef ADIOS2_HAVE_DATAMAN
        return std::make_shared<DataManReader>(*this, name, accessMode, mpiComm,
                                               method);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with "
            "Dataman library, can't Open DataManReader\n");
#endif
    }
    else if (type == "ADIOS1Writer")
    {
#ifdef ADIOS2_HAVE_ADIOS1
        return std::make_shared<ADIOS1Writer>(*this, name, accessMode, mpiComm,
                                              method);
#else
        throw std::invalid_argument(
            "ERROR: this version didn't compile with ADIOS "
            "1.x library, can't Open ADIOS1Writer\n");
#endif
    }
    else if (type == "Vis")
    {
        // return std::make_shared<Vis>( *this, name, accessMode, mpiComm,
        // method,
        // iomode, timeout_sec, m_DebugMode, method.m_nThreads );
    }
    else if (type == "HDF5Writer") // -junmin
    {
#ifdef ADIOS2_HAVE_HDF5
        return std::make_shared<HDF5Writer>(*this, name, accessMode, mpiComm,
                                            method);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else if (type == "HDF5Reader") // -Junmin
    {
#ifdef ADIOS2_HAVE_HDF5
        return std::make_shared<HDF5Reader>(*this, name, accessMode, mpiComm,
                                            method);
#else
        throw std::invalid_argument("ERROR: this version didn't compile with "
                                    "HDF5 library, can't use HDF5\n");
#endif
    }
    else
    {
        if (m_DebugMode == true)
        {
            throw std::invalid_argument("ERROR: method type " + type +
                                        " not supported for " + name +
                                        ", in call to Open\n");
        }
    }

    return nullptr; // if debug mode is off
}

std::shared_ptr<Engine> ADIOS::Open(const std::string &name,
                                    const std::string accessMode,
                                    const Method &method)
{
    return Open(name, accessMode, m_MPIComm, method);
}

std::shared_ptr<Engine> ADIOS::Open(const std::string &name,
                                    const std::string accessMode,
                                    MPI_Comm mpiComm,
                                    const std::string methodName)
{
    auto itMethod = m_Methods.find(methodName);

    if (m_DebugMode == true)
    {
        CheckMethod(itMethod, methodName, " in call to Open\n");
    }

    return Open(name, accessMode, mpiComm, itMethod->second);
}

std::shared_ptr<Engine> ADIOS::Open(const std::string &name,
                                    const std::string accessMode,
                                    const std::string methodName)
{
    return Open(name, accessMode, m_MPIComm, methodName);
}

std::shared_ptr<Engine> ADIOS::OpenFileReader(const std::string &fileName,
                                              MPI_Comm mpiComm,
                                              const Method &method)

{
    return Open(fileName, "r", mpiComm, method);
}

std::shared_ptr<Engine> ADIOS::OpenFileReader(const std::string &name,
                                              MPI_Comm mpiComm,
                                              const std::string methodName)

{
    auto itMethod = m_Methods.find(methodName);

    if (m_DebugMode == true)
    {
        CheckMethod(itMethod, methodName, " in call to Open\n");
    }

    return Open(name, "r", m_MPIComm, itMethod->second);
}

VariableCompound &ADIOS::GetVariableCompound(const std::string &name)
{
    return m_Compound.at(GetVariableIndex<void>(name));
}

void ADIOS::MonitorVariables(std::ostream &logStream)
{
    logStream << "\tVariable \t Type\n";

    for (auto &variablePair : m_Variables)
    {
        const std::string name(variablePair.first);
        const std::string type(variablePair.second.first);

        if (type == GetType<char>())
        {
            GetVariable<char>(name).Monitor(logStream);
        }
        else if (type == GetType<unsigned char>())
        {
            GetVariable<unsigned char>(name).Monitor(logStream);
        }
        else if (type == GetType<short>())
        {
            GetVariable<short>(name).Monitor(logStream);
        }
        else if (type == GetType<unsigned short>())
        {
            GetVariable<unsigned short>(name).Monitor(logStream);
        }
        else if (type == GetType<int>())
        {
            GetVariable<int>(name).Monitor(logStream);
        }
        else if (type == GetType<unsigned int>())
        {
            GetVariable<unsigned int>(name).Monitor(logStream);
        }
        else if (type == GetType<long int>())
        {
            GetVariable<long int>(name).Monitor(logStream);
        }
        else if (type == GetType<unsigned long int>())
        {
            GetVariable<unsigned long int>(name).Monitor(logStream);
        }
        else if (type == GetType<long long int>())
        {
            GetVariable<long long int>(name).Monitor(logStream);
        }
        else if (type == GetType<unsigned long long int>())
        {
            GetVariable<unsigned long long int>(name).Monitor(logStream);
        }
        else if (type == GetType<float>())
        {
            GetVariable<float>(name).Monitor(logStream);
        }
        else if (type == GetType<double>())
        {
            GetVariable<double>(name).Monitor(logStream);
        }
        else if (type == GetType<long double>())
        {
            GetVariable<long double>(name).Monitor(logStream);
        }
        else if (type == GetType<std::complex<float>>())
        {
            GetVariable<std::complex<float>>(name).Monitor(logStream);
        }
        else if (type == GetType<std::complex<double>>())
        {
            GetVariable<std::complex<double>>(name).Monitor(logStream);
        }
        else if (type == GetType<std::complex<long double>>())
        {
            GetVariable<std::complex<long double>>(name).Monitor(logStream);
        }
    }
}

// PRIVATE FUNCTIONS BELOW
void ADIOS::CheckVariableInput(const std::string &name,
                               const Dims &dimensions) const
{
    if (m_DebugMode == true)
    {
        if (m_Variables.count(name) == 1)
        {
            throw std::invalid_argument(
                "ERROR: variable " + name +
                " already exists, in call to DefineVariable\n");
        }

        if (dimensions.empty() == true)
        {
            throw std::invalid_argument(
                "ERROR: variable " + name +
                " dimensions can't be empty, in call to DefineVariable\n");
        }
    }
}

void ADIOS::CheckVariableName(
    std::map<std::string, std::pair<std::string, unsigned int>>::const_iterator
        itVariable,
    const std::string &name, const std::string hint) const
{
    if (m_DebugMode == true)
    {
        if (itVariable == m_Variables.end())
        {
            throw std::invalid_argument("ERROR: variable " + name +
                                        " does not exist " + hint + "\n");
        }
    }
}

void ADIOS::CheckMethod(std::map<std::string, Method>::const_iterator itMethod,
                        const std::string methodName,
                        const std::string hint) const
{
    if (itMethod == m_Methods.end())
    {
        throw std::invalid_argument("ERROR: method " + methodName +
                                    " not found " + hint + "\n");
    }
}

//------------------------------------------------------------------------------

// Explicitly instantiate the necessary template implementations
#define define_template_instantiation(T)                                       \
    template Variable<T> &ADIOS::DefineVariable<T>(                            \
        const std::string &, const Dims, const Dims, const Dims);              \
                                                                               \
    template Variable<T> &ADIOS::GetVariable<T>(const std::string &);

ADIOS_FOREACH_TYPE_1ARG(define_template_instantiation)
template unsigned int ADIOS::GetVariableIndex<void>(const std::string &);
#undef define_template_instatiation

//------------------------------------------------------------------------------

} // end namespace adios
