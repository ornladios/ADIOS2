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

#include <mpi4py/mpi4py.h>
#include <pybind11.h>

#include "bindings/python/source/ADIOSPy.h"
#include "bindings/python/source/EnginePy.h"
#include "bindings/python/source/IOPy.h"

namespace py = pybind11;

adios::ADIOSPy ADIOSPy(py::object py_comm, const bool debugMode)
{
    MPI_Comm *comm_p = PyMPIComm_Get(py_comm.ptr());

    if (debugMode)
    {
        if (comm_p == NULL)
        {
            py::error_already_set(
                "ERROR: mpi4py communicator in ADIOS in null\n");
        }
    }
    return adios::ADIOSPy(*comm_p, debugMode);
}

PYBIND11_PLUGIN(ADIOSPy)
{
    if (import_mpi4py() < 0)
    {
        throw std::runtime_error(
            "ERROR: mpi4py not loaded correctly\n"); /* Python 2.X */
    }

    py::module m("ADIOSPy", "ADIOS Python bindings using pybind11");

    m.def("ADIOSPy", &ADIOSPy, "Function that creates an ADIOS object");

    py::class_<adios::ADIOSPy>(m, "ADIOS")
        .def("HelloMPI", &adios::ADIOSPy::HelloMPI)
        //.def("DefineVariable", &adios::ADIOSPy::DefineVariablePy)
        .def("DeclareIO", &adios::ADIOSPy::DeclareIOPy,
             py::return_value_policy::reference_internal)
        //.def("Open", &adios::ADIOSPy::OpenPy);

        py::class_<adios::VariablePy>(m, "Variable")
        .def("SetLocalDimensions", &adios::VariablePy::SetLocalDimensions)
        .def("GetLocalDimensions", &adios::VariablePy::GetLocalDimensions);

    py::class_<adios::IOPy>(m, "IO")
        .def("SetParameters", &adios::IOPy::SetParametersPy)
        .def("AddTransport", &adios::IOPy::AddTransportPy)
        .def("PrintAll", &adios::MethodPy::PrintAll);

    // Engine
    py::class_<adios::EnginePy>(m, "Engine")
        .def("Write", &adios::EnginePy::WritePy)
        .def("Advance", &adios::EnginePy::WritePy)
        .def("Close", &adios::EnginePy::Close);

    return m.ptr();
}
