/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * ADIOSPy.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "py11ADIOS.h"

#include "adios2/ADIOSMPI.h"

namespace adios2
{
namespace py11
{

#ifdef ADIOS2_HAVE_MPI
ADIOS::ADIOS(const std::string configFile, MPI_Comm mpiComm,
             const bool debugMode)
: m_DebugMode(debugMode), m_ADIOS(std::make_shared<adios2::core::ADIOS>(
                              configFile, mpiComm, debugMode, "Python"))
{
}

ADIOS::ADIOS(MPI_Comm mpiComm, const bool debugMode)
: ADIOS("", mpiComm, debugMode)
{
}
#else
ADIOS::ADIOS(const std::string configFile, const bool debugMode)
: ADIOS(configFile, debugMode)
{
}

ADIOS::ADIOS(const bool debugMode) : ADIOS("", debugMode) {}
#endif

ADIOS::operator bool() const noexcept { return m_ADIOS ? true : false; }

IO ADIOS::DeclareIO(const std::string name)
{
    return IO(m_ADIOS->DeclareIO(name), m_DebugMode);
}

IO ADIOS::AtIO(const std::string name)
{
    return IO(m_ADIOS->AtIO(name), m_DebugMode);
}

void ADIOS::FlushAll() { m_ADIOS->FlushAll(); }

} // end namespace py11
} // end namespace adios2
