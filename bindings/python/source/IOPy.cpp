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

#include <mpi4py/mpi4py.h>

#include "adiosPyFunctions.h" //PyObjectToMPIComm

namespace adios
{

IOPy::IOPy(IO &io, const bool debugMode) : m_IO(io), m_DebugMode(debugMode)
{
    m_IO.m_HostLanguage = "Python";
}

void IOPy::SetEngine(const std::string engine) { m_IO.SetEngine(engine); }

void IOPy::SetParameters(const pyKwargs &kwargs) noexcept
{
    m_IO.SetParameters(KwargsToParams(kwargs));
}

unsigned int IOPy::AddTransport(const std::string type,
                                const pyKwargs &kwargs) noexcept
{
    return m_IO.AddTransport(type, KwargsToParams(kwargs));
}

VariablePy &IOPy::DefineVariable(const std::string &name, const pyList shape,
                                 const pyList start, const pyList count,
                                 const bool isConstantDims)
{
    if (m_DebugMode)
    {
        if (m_Variables.count(name) == 1)
        {
            throw std::invalid_argument("ERROR: variable " + name +
                                        " already exists, use GetVariable, in "
                                        "call to DefineVariable\n");
        }
    }

    auto itVariableEmplace =
        m_Variables.emplace(name, VariablePy(name, shape, start, count,
                                             isConstantDims, m_DebugMode));
    return itVariableEmplace.first->second;
}

VariablePy &IOPy::GetVariable(const std::string &name)
{
    auto itVariable = m_Variables.find(name);

    if (m_DebugMode)
    {
        if (itVariable == m_Variables.end())
        {
            throw std::invalid_argument("ERROR: variable " + name +
                                        " doesn't exist, in "
                                        "call to GetVariable\n");
        }
    }
    return itVariable->second;
}

EnginePy IOPy::Open(const std::string &name, const int openMode,
                    adios::pyObject &object)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(object.ptr());

    if (import_mpi4py() < 0)
    {
        throw std::logic_error("ERROR: could not import mpi4py "
                               "communicator, in call to ADIOS "
                               "constructor\n");
    }

    if (mpiCommPtr == nullptr)
    {
        throw std::runtime_error("ERROR: mpi4py communicator is null, in call "
                                 "to ADIOS constructor\n");
    }

    return EnginePy(m_IO, name, static_cast<adios::OpenMode>(openMode),
                    *mpiCommPtr);
}

EnginePy IOPy::Open(const std::string &name, const int openMode)
{
    return EnginePy(m_IO, name, static_cast<adios::OpenMode>(openMode),
                    m_IO.m_MPIComm);
}

} // end namespace adios
