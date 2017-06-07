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

#include "bindings/python/source/adiosPyTypes.h"

namespace adios
{

ADIOSPy::ADIOSPy(MPI_Comm mpiComm, const bool debug)
: m_DebugMode(debug), ADIOS(mpiComm, debug)
{
}

IOPy &ADIOSPy::DeclareIO(const std::string name)
{
    return IOPy(m_ADIOS.DeclareIO(name), m_DebugMode);
}

// VariablePy ADIOSPy::DefineVariablePy(const std::string name,
//                                     const pyList localDimensionsPy,
//                                     const pyList globalDimensionsPy,
//                                     const pyList globalOffsetsPy)
//{
//    if (m_DebugMode == true)
//    {
//        if (m_VariablesPyNames.count(name) == 1)
//            throw std::invalid_argument("ERROR: Variable " + name +
//                                        " is already defined\n");
//    }
//
//    m_VariablesPyNames.insert(name);
//    return VariablePy(name, localDimensionsPy, globalDimensionsPy,
//                      globalOffsetsPy);
//}
//
// void ADIOSPy::DefineVariableType(VariablePy &variablePy) {}
//
// EnginePy ADIOSPy::OpenPy(const std::string name, const std::string
// accessMode,
//                         const MethodPy &method, pyObject py_comm)
//{
//    EnginePy enginePy(*this);
//
//    bool isEmpty = IsEmpty(py_comm);
//
//    if (isEmpty == true) // None
//    {
//        enginePy.m_Engine = Open(name, accessMode, method);
//    }
//    else
//    {
//        if (import_mpi4py() < 0)
//            throw std::logic_error(
//                "ERROR: could not import mpi4py communicator in Open " + name
//                +
//                "\n");
//        MPI_Comm *comm_p = PyMPIComm_Get(py_comm.ptr());
//        if (comm_p == nullptr)
//            throw std::invalid_argument(
//                "ERROR: MPI communicator is nullptr in Open " + name + "\n");
//
//        enginePy.m_Engine = Open(name, accessMode, *comm_p, method);
//    }
//
//    return enginePy;
//}

} // end namespace
