/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * gluePyBind11.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: wfg
 */

#include <stdexcept>

#include <mpi4py/mpi4py.h>

#include "pybind11/pybind11.h"

#include "ADIOSPy.h"
#include "EnginePy.h"

namespace py = pybind11;

adios::ADIOSPy ADIOSPy(py::object py_comm, const bool debug)
{
    MPI_Comm *comm_p = PyMPIComm_Get(py_comm.ptr());
    if (comm_p == NULL)
        py::error_already_set();
    return adios::ADIOSPy(*comm_p, debug);
}

PYBIND11_PLUGIN(ADIOSPy)
{
    if (import_mpi4py() < 0)
        throw std::runtime_error(
            "ERROR: mpi4py not loaded correctly\n"); /* Python 2.X */

    py::module m("ADIOSPy", "ADIOS Python bindings using pybind11");

    m.def("ADIOSPy", &ADIOSPy, "Function that creates an ADIOS object");

    py::class_<adios::ADIOSPy>(m, "ADIOS")
        .def("HelloMPI", &adios::ADIOSPy::HelloMPI)
        .def("DefineVariable", &adios::ADIOSPy::DefineVariablePy)
        .def("DeclareMethod", &adios::ADIOSPy::DeclareMethodPy,
             py::return_value_policy::reference_internal)
        .def("Open", &adios::ADIOSPy::OpenPy);

    py::class_<adios::VariablePy>(m, "Variable")
        .def("SetLocalDimensions", &adios::VariablePy::SetLocalDimensions)
        .def("GetLocalDimensions", &adios::VariablePy::GetLocalDimensions);

    py::class_<adios::MethodPy>(m, "Method")
        .def("SetParameters", &adios::MethodPy::SetParametersPyBind11)
        .def("AddTransport", &adios::MethodPy::AddTransportPyBind11)
        .def("PrintAll", &adios::MethodPy::PrintAll);

    // Engine
    py::class_<adios::EnginePy>(m, "Engine")
        .def("Write", &adios::EnginePy::WritePy)
        .def("Advance", &adios::EnginePy::WritePy)
        .def("Close", &adios::EnginePy::Close);

    return m.ptr();
}
