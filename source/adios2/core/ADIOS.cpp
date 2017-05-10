/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOS.cpp
 *
 *  Created on: Sep 29, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOS.h"

/// \cond EXCLUDE_FROM_DOXYGEN
#include <fstream>
#include <ios> //std::ios_base::failure
#include <iostream>
#include <sstream>
#include <utility>
/// \endcond

namespace adios
{

ADIOS::ADIOS(const bool debugMode) : m_DebugMode(debugMode) {}

ADIOS::ADIOS(const std::string configFile, const bool debugMode)
: m_ConfigFile(configFile), m_DebugMode(debugMode)
{
}

ADIOS::ADIOS(const std::string configFile, MPI_Comm mpiComm,
             const bool debugMode)
: m_MPIComm(mpiComm), m_ConfigFile(configFile), m_DebugMode(debugMode)
{
    if (m_DebugMode == true)
    {
        CheckMPI();
    }
    // XML to be implemented later
    // InitXML( m_XMLConfigFile, m_MPIComm, m_DebugMode, m_HostLanguage,
    // m_Transforms, m_Groups );
}

ADIOS::ADIOS(MPI_Comm mpiComm, const bool debugMode)
: m_MPIComm(mpiComm), m_DebugMode(debugMode)
{
    if (m_DebugMode == true)
    {
        CheckMPI();
    }
}

IO &ADIOS::DeclareIO(const std::string ioName)
{
    auto itIO = m_IOs.find(ioName);

    if (itIO != m_IOs.end()) // exists
    {
        if (m_DebugMode == true)
        {
            if (itIO->second.InConfigFile() == false)
            {
                throw std::invalid_argument(
                    "ERROR: IO class object with name " + ioName +
                    " previously declared, name must be unique "
                    " , in call to DeclareIO\n");
            }
        }
        return itIO->second;
    }

    // doesn't exist, then create
    m_IOs.emplace(ioName, IO(ioName, m_MPIComm, false, m_DebugMode));
    return m_IOs.at(ioName);
}

// PRIVATE FUNCTIONS
void ADIOS::CheckMPI() const
{
    if (m_MPIComm == MPI_COMM_NULL)
    {
        throw std::ios_base::failure(
            "ERROR: engine communicator is MPI_COMM_NULL,"
            " in call to ADIOS constructor\n");
    }
}

} // end namespace adios
