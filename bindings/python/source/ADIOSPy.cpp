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

#include "adiosPyTypes.h"

#include <iostream>

namespace adios
{

ADIOSPy::ADIOSPy(MPI_Comm mpiComm, const bool debug)
: m_DebugMode(debug), m_ADIOS(mpiComm, debug)
{
}

IOPy ADIOSPy::DeclareIO(const std::string name)
{
    std::cout << "Declaring IO " << name << "\n";

    return IOPy(m_ADIOS.DeclareIO(name), m_DebugMode);
}

} // end namespace adios
