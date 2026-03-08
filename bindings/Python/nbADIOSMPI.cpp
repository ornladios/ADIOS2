/*
 * SPDX-FileCopyrightText: 2026 Oak Ridge National Laboratory and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "nbADIOS.h"

#include "adios2/helper/adiosCommMPI.h"

namespace adios2
{
namespace py11
{

ADIOS::ADIOS(const std::string &configFile, MPI4PY_Comm mpiComm)
: m_ADIOS(std::make_shared<adios2::core::ADIOS>(configFile, helper::CommDupMPI(mpiComm), "Python"))
{
}

ADIOS::ADIOS(MPI4PY_Comm mpiComm) : ADIOS("", mpiComm) {}

} // end namespace py11
} // end namespace adios2
