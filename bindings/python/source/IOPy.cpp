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

namespace adios
{

IOPy::IOPy(IO &io, const bool debugMode) : m_IO(io), m_DebugMode(debugMode)
{
    m_IO.m_HostLanguage = "Python";
}

void IOPy::SetParameters(const pyKwargs &kwargs)
{
    m_IO.SetParameters(KwargsToParams(kwargs));
}

unsigned int IOPy::AddTransport(const std::string type, const pyKwargs &kwargs)
{
    return m_IO.AddTransport(type, KwargsToParams(kwargs));
}

VariablePy IOPy::DefineVariable(const std::string &name, const pyList shape,
                                const pyList start, const pyList count,
                                const bool isConstantDims)
{
    return VariablePy(name, shape, start, count, isConstantDims, m_DebugMode);
}

EnginePy IOPy::Open(const std::string &name, const int openMode)
{
    //    MPI_Comm *mpiCommPtr = PyMPIComm_Get(pyMPIComm.ptr());
    //
    //    if (m_DebugMode)
    //    {
    //        if (mpiCommPtr == NULL)
    //        {
    //            throw std::runtime_error("ERROR: mpi4py communicator for
    //            engine " +
    //                                     name + " is null, in call to IO
    //                                     Open\n");
    //        }
    //    }
    return EnginePy(m_IO, name, static_cast<adios::OpenMode>(openMode),
                    m_IO.m_MPIComm);
}

// EnginePy IOPy::Open(const std::string &name, const OpenMode openMode)
//{
//    return EnginePy(m_IO, name, openMode, m_IO.m_MPIComm);
//}

} // end namespace
