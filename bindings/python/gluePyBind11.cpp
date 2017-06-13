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
#include <pybind11/pybind11.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi4py/mpi4py.h>
#endif

#include "ADIOSPy.h"
#include "EnginePy.h"
#include "IOPy.h"
#include "VariablePy.h"
#include "adiosPyFunctions.h"
#include "adiosPyTypes.h"

#ifdef ADIOS2_HAVE_MPI
adios::ADIOSPy ADIOSPyInit(adios::pyObject &object, const bool debugMode)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(object.ptr());

    if (import_mpi4py() < 0)
    {
        throw std::runtime_error("ERROR: could not import mpi4py "
                                 "communicator, in call to ADIOS "
                                 "constructor\n");
    }

    if (mpiCommPtr == nullptr)
    {
        throw std::runtime_error("ERROR: mpi4py communicator is null, in call "
                                 "to ADIOS constructor\n");
    }
    return adios::ADIOSPy(*mpiCommPtr, debugMode);
}
#else
adios::ADIOSPy ADIOSPyInit(const bool debugMode)
{
    return adios::ADIOSPy(debugMode);
}
#endif

PYBIND11_PLUGIN(adios2)
{
#ifdef ADIOS2_HAVE_MPI
    if (import_mpi4py() < 0)
    {
        throw std::runtime_error(
            "ERROR: mpi4py not loaded correctly\n"); /* Python 2.X */
    }
#endif

    pybind11::module m("adios2", "ADIOS2 Python bindings using pybind11");
    m.attr("DebugON") = true;
    m.attr("DebugOFF") = false;
    m.attr("ConstantDims") = true;
    m.attr("OpenModeWrite") = static_cast<int>(adios::OpenMode::Write);
    m.attr("OpenModeRead") = static_cast<int>(adios::OpenMode::Read);
    m.attr("OpenModeAppend") = static_cast<int>(adios::OpenMode::Append);
    m.attr("OpenModeReadWrite") = static_cast<int>(adios::OpenMode::ReadWrite);
    m.def("ADIOS", &ADIOSPyInit, "Function that creates an ADIOS class object");

    pybind11::class_<adios::ADIOSPy>(m, "ADIOSPy")
        .def("DeclareIO", &adios::ADIOSPy::DeclareIO);

    pybind11::class_<adios::IOPy>(m, "IOPy")
        .def("SetEngine", &adios::IOPy::SetEngine)
        .def("SetParameters", &adios::IOPy::SetParameters)
        .def("AddTransport", &adios::IOPy::AddTransport)
        .def("DefineVariable", &adios::IOPy::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("shape") = adios::pyList(),
             pybind11::arg("start") = adios::pyList(),
             pybind11::arg("count") = adios::pyList(),
             pybind11::arg("isConstantDims") = false)
        .def("GetVariable", &adios::IOPy::GetVariable,
             pybind11::return_value_policy::reference_internal)
        .def("Open", (adios::EnginePy (adios::IOPy::*)(const std::string &,
                                                       const int)) &
                         adios::IOPy::Open);

    pybind11::class_<adios::VariablePy>(m, "VariablePy")
        .def("SetDimensions", &adios::VariablePy::SetDimensions);

    pybind11::class_<adios::EnginePy>(m, "EnginePy")
        .def("Write", &adios::EnginePy::Write)
        .def("Advance", &adios::EnginePy::Advance,
             pybind11::arg("timeoutSeconds") = 0.)
        .def("Close", &adios::EnginePy::Close,
             pybind11::arg("transportIndex") = -1);

    return m.ptr();
}
