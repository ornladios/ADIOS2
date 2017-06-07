/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * IOPy.cpp
 *
 *  Created on: Mar 14, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "IOPy.h"

namespace adios
{

IOPy::IOPy(IO &io, const bool debugMode) : m_IO(io), m_DebugMode(debugMode) {}

void IOPy::SetParameters(const pybind11::kwargs kwargs)
{ // transform to vector and call m_IO SetParameters
}

unsigned int IOPy::AddTransport(const std::string type,
                                const pybind11::kwargs kwargs)
{
    // transform to vector and call AddTransport
    return -1;
}

} // end namespace
