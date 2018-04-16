/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSPy.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "ADIOSPy.h"

#include "adios2/ADIOSMPI.h"

namespace adios2
{

ADIOSPy::ADIOSPy(const std::string configFile, MPI_Comm mpiComm,
                 const bool debugMode)
: m_DebugMode(debugMode), m_ADIOS(std::make_shared<adios2::ADIOS>(
                              configFile, mpiComm, debugMode, "Python"))
{
}

ADIOSPy::ADIOSPy(MPI_Comm mpiComm, const bool debugMode)
: ADIOSPy("", mpiComm, debugMode)
{
}

ADIOSPy::ADIOSPy(const std::string configFile, const bool debugMode)
: ADIOSPy(configFile, MPI_COMM_SELF, debugMode)
{
}

ADIOSPy::ADIOSPy(const bool debugMode) : ADIOSPy("", MPI_COMM_SELF, debugMode)
{
}

IOPy ADIOSPy::DeclareIO(const std::string name)
{
    return IOPy(m_ADIOS->DeclareIO(name), m_DebugMode);
}

IOPy ADIOSPy::AtIO(const std::string name)
{
    return IOPy(m_ADIOS->AtIO(name), m_DebugMode);
}

void ADIOSPy::FlushAll() { m_ADIOS->FlushAll(); }

} // end namespace adios2
