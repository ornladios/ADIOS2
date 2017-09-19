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
adios2::ADIOSPy ADIOSPyInitConfig(const std::string configFile,
                                  adios2::pyObject &object,
                                  const bool debugMode)
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
    return adios2::ADIOSPy(configFile, *mpiCommPtr, debugMode);
}

adios2::ADIOSPy ADIOSPyInit(adios2::pyObject &object, const bool debugMode)
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
    return adios2::ADIOSPy(*mpiCommPtr, debugMode);
}
#else
adios2::ADIOSPy ADIOSPyInitConfig(const std::string configFile,
                                  const bool debugMode)
{
    return adios2::ADIOSPy(configFile, debugMode);
}

adios2::ADIOSPy ADIOSPyInit(const bool debugMode)
{
    return adios2::ADIOSPy(debugMode);
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
    m.attr("OpenModeWrite") = static_cast<int>(adios2::OpenMode::Write);
    m.attr("OpenModeRead") = static_cast<int>(adios2::OpenMode::Read);
    m.attr("OpenModeAppend") = static_cast<int>(adios2::OpenMode::Append);
    m.attr("OpenModeReadWrite") = static_cast<int>(adios2::OpenMode::ReadWrite);
    m.def("ADIOS", &ADIOSPyInit, "Function that creates an ADIOS class object");
    m.def("ADIOS", &ADIOSPyInitConfig,
          "Function that creates an ADIOS class object using a config file");

    pybind11::class_<adios2::ADIOSPy>(m, "ADIOSPy")
        .def("DeclareIO", &adios2::ADIOSPy::DeclareIO);

    pybind11::class_<adios2::IOPy>(m, "IOPy")
        .def("SetEngine", &adios2::IOPy::SetEngine)
        .def("SetParameters", &adios2::IOPy::SetParameters)
        .def("AddTransport", &adios2::IOPy::AddTransport)
        .def("DefineVariable", &adios2::IOPy::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("shape") = adios2::pyList(),
             pybind11::arg("start") = adios2::pyList(),
             pybind11::arg("count") = adios2::pyList(),
             pybind11::arg("isConstantDims") = false)
        .def("GetVariable", &adios2::IOPy::GetVariable,
             pybind11::return_value_policy::reference_internal)
        .def("Open", (adios2::EnginePy (adios2::IOPy::*)(const std::string &,
                                                         const int)) &
                         adios2::IOPy::Open);

    pybind11::class_<adios2::VariablePy>(m, "VariablePy")
        .def("SetDimensions", &adios2::VariablePy::SetDimensions);

    pybind11::class_<adios2::EnginePy>(m, "EnginePy")
        .def("Write", &adios2::EnginePy::Write)
        .def("Advance", &adios2::EnginePy::Advance,
             pybind11::arg("timeoutSeconds") = 0.)
        .def("Close", &adios2::EnginePy::Close,
             pybind11::arg("transportIndex") = -1);

    return m.ptr();
}
