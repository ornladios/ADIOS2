/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * gluePyBind11.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <stdexcept>

#include <adios2.h>
#include <mpi4py/mpi4py.h>
#include <pybind11/pybind11.h>

#include "ADIOSPy.h"
#include "EnginePy.h"
#include "IOPy.h"
#include "VariablePy.h"
#include "adiosPyTypes.h"

adios::ADIOSPy ADIOSPyInit(adios::pyObject py_comm, const bool debugMode)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(py_comm.ptr());

    if (debugMode)
    {
        if (mpiCommPtr == NULL)
        {
            throw std::runtime_error(
                "ERROR: mpi4py communicator is null, in call "
                "to ADIOS constructor\n");
        }
    }
    return adios::ADIOSPy(*mpiCommPtr, debugMode);
}

PYBIND11_PLUGIN(adios2py)
{
    if (import_mpi4py() < 0)
    {
        throw std::runtime_error(
            "ERROR: mpi4py not loaded correctly\n"); /* Python 2.X */
    }

    pybind11::module m("adios2py", "ADIOS2 Python bindings using pybind11");
    m.attr("DebugON") = true;
    m.attr("DebugOFF") = false;
    m.attr("Write") = static_cast<int>(adios::OpenMode::Write);
    m.attr("Read") = static_cast<int>(adios::OpenMode::Read);
    m.attr("Append") = static_cast<int>(adios::OpenMode::Append);
    m.attr("ReadWrite") = static_cast<int>(adios::OpenMode::ReadWrite);
    m.def("ADIOS", &ADIOSPyInit, "Function that creates an ADIOS object");

    pybind11::class_<adios::ADIOSPy>(m, "ADIOSPy")
        .def("DeclareIO", &adios::ADIOSPy::DeclareIO);

    pybind11::class_<adios::IOPy>(m, "IOPy")
        .def("SetParameters", &adios::IOPy::SetParameters)
        .def("AddTransport", &adios::IOPy::AddTransport)
        .def("DefineVariable", &adios::IOPy::DefineVariable)
        .def("Open", &adios::IOPy::Open);

    pybind11::class_<adios::VariablePy>(m, "VariablePy")
        .def("SetDimensions", &adios::VariablePy::SetDimensions);

    pybind11::class_<adios::EnginePy>(m, "EnginePy")
        .def("Write", &adios::EnginePy::Write)
        .def("Advance", &adios::EnginePy::Advance)
        .def("Close", &adios::EnginePy::Close);

    // Trying overloaded function
    // (adios::EnginePy (adios::IOPy::*)(
    //                 const std::string &, const adios::OpenMode,
    //                 adios::pyObject)) &
    //                 &adios::IOPy::Open)
    //        .def("Open", (adios::EnginePy (adios::IOPy::*)(const std::string
    //        &,
    //                                                       const
    //                                                       adios::OpenMode)) &
    //                         &adios::IOPy::Open);

    return m.ptr();
}
