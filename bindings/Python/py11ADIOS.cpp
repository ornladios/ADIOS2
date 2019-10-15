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

#ifdef ADIOS2_HAVE_MPI
#include "adios2/helper/adiosCommMPI.h"
#endif

namespace adios2
{
namespace py11
{

#ifdef ADIOS2_HAVE_MPI
ADIOS::ADIOS(const std::string &configFile, MPI4PY_Comm mpiComm,
             const bool debugMode)
: m_ADIOS(std::make_shared<adios2::core::ADIOS>(
      configFile, helper::CommFromMPI(mpiComm), debugMode, "Python"))
{
}

ADIOS::ADIOS(MPI4PY_Comm mpiComm, const bool debugMode)
: ADIOS("", mpiComm, debugMode)
{
}
#endif
ADIOS::ADIOS(const std::string &configFile, const bool debugMode)
: m_ADIOS(
      std::make_shared<adios2::core::ADIOS>(configFile, debugMode, "Python"))
{
}

ADIOS::ADIOS(const bool debugMode) : ADIOS("", debugMode) {}

ADIOS::operator bool() const noexcept { return m_ADIOS ? true : false; }

IO ADIOS::DeclareIO(const std::string name)
{
    CheckPointer("for io name " + name + ", in call to ADIOS::DeclareIO");
    return IO(&m_ADIOS->DeclareIO(name));
}

IO ADIOS::AtIO(const std::string name)
{
    CheckPointer("for io name " + name + ", in call to ADIOS::AtIO");
    return IO(&m_ADIOS->AtIO(name));
}

Operator ADIOS::DefineOperator(const std::string name, const std::string type,
                               const Params &parameters)
{
    CheckPointer("for operator name " + name +
                 ", in call to ADIOS::DefineOperator");
    return Operator(&m_ADIOS->DefineOperator(name, type, parameters));
}

Operator ADIOS::InquireOperator(const std::string name)
{
    CheckPointer("for operator name " + name + ", in call to InquireOperator");
    return Operator(m_ADIOS->InquireOperator(name));
}

void ADIOS::FlushAll()
{
    CheckPointer("in call to ADIOS::FlushAll");
    m_ADIOS->FlushAll();
}

bool ADIOS::RemoveIO(const std::string name)
{
    CheckPointer("in call to ADIOS::RemoveIO");
    return m_ADIOS->RemoveIO(name);
}

void ADIOS::RemoveAllIOs()
{
    CheckPointer("in call to ADIOS::RemoveAllIOs");
    m_ADIOS->RemoveAllIOs();
}

// PRIVATE
void ADIOS::CheckPointer(const std::string hint)
{
    if (!m_ADIOS)
    {
        throw std::invalid_argument("ERROR: invalid ADIOS object, did you call "
                                    "any of the ADIOS explicit "
                                    "constructors?, " +
                                    hint + "\n");
    }
}

} // end namespace py11
} // end namespace adios2
