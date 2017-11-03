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
#include <pybind11/stl.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi4py/mpi4py.h>
#endif

#include "ADIOSPy.h"
#include "EnginePy.h"
#include "IOPy.h"

#ifdef ADIOS2_HAVE_MPI
adios2::ADIOSPy ADIOSPyInitConfig(const std::string configFile,
                                  pybind11::object &object,
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

adios2::ADIOSPy ADIOSPyInit(pybind11::object &object, const bool debugMode)
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

PYBIND11_MODULE(adios2, m)
{
#ifdef ADIOS2_HAVE_MPI
    if (import_mpi4py() < 0)
    {
        throw std::runtime_error(
            "ERROR: mpi4py not loaded correctly\n"); /* Python 2.X */
    }
#endif

    // pybind11::module m("adios2", "ADIOS2 Python bindings using pybind11");
    m.doc() = "ADIOS2 Python bindings powered by pybind11";

    m.attr("DebugON") = true;
    m.attr("DebugOFF") = false;
    m.attr("ConstantDims") = true;
    // enum classes
    pybind11::enum_<adios2::Mode>(m, "Mode")
        .value("Write", adios2::Mode::Write)
        .value("Read", adios2::Mode::Read)
        .value("Append", adios2::Mode::Append)
        .export_values();

    pybind11::enum_<adios2::StepMode>(m, "StepMode")
        .value("Append", adios2::StepMode::Append)
        .value("Update", adios2::StepMode::Update)
        .value("NextAvailable", adios2::StepMode::NextAvailable)
        .value("LatestAvailable", adios2::StepMode::LatestAvailable)
        .export_values();

    pybind11::enum_<adios2::StepStatus>(m, "StepStatus")
        .value("OK", adios2::StepStatus::OK)
        .value("NotReady", adios2::StepStatus::NotReady)
        .value("EndOfStream", adios2::StepStatus::EndOfStream)
        .value("OtherError", adios2::StepStatus::OtherError)
        .export_values();
#ifdef ADIOS2_HAVE_MPI
    m.def("ADIOS", &ADIOSPyInit, "Function that creates an ADIOS class object",
          pybind11::arg("object") = nullptr, pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSPyInitConfig, "Function that creates an ADIOS class "
                                       "object using a config file",
          pybind11::arg("configFile") = "", pybind11::arg("object") = nullptr,
          pybind11::arg("debugMode") = true);
#else
    m.def("ADIOS", &ADIOSPyInit,
          "Function that creates an ADIOS class object in non MPI mode",
          pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSPyInitConfig,
          "Function that creates an ADIOS class "
          "object using a config file in non MPI mode",
          pybind11::arg("configFile") = "", pybind11::arg("debugMode") = true);
#endif

    pybind11::class_<adios2::ADIOSPy>(m, "ADIOSPy")
        .def("DeclareIO", &adios2::ADIOSPy::DeclareIO);

    pybind11::class_<adios2::VariableBase>(m, "Variable")
        .def("SetSelection", &adios2::VariableBase::SetSelection);

    pybind11::class_<adios2::IOPy>(m, "IOPy")
        .def("SetEngine", &adios2::IOPy::SetEngine)
        .def("SetParameters", &adios2::IOPy::SetParameters)
        //.def("AddTransport", &adios2::IOPy::AddTransport)
        .def("DefineVariable", &adios2::IOPy::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("shape") = adios2::Dims(),
             pybind11::arg("start") = adios2::Dims(),
             pybind11::arg("count") = adios2::Dims(),
             pybind11::arg("isConstantDims") = false,
             pybind11::arg("array") = pybind11::array())
        .def("InquireVariable", &adios2::IOPy::InquireVariable,
             pybind11::return_value_policy::reference_internal)
        .def("Open", (adios2::EnginePy (adios2::IOPy::*)(const std::string &,
                                                         const int)) &
                         adios2::IOPy::Open);

    pybind11::class_<adios2::EnginePy>(m, "EnginePy")
        .def("PutSync", &adios2::EnginePy::PutSync)
        .def("EndStep", &adios2::EnginePy::EndStep)
        .def("Close", &adios2::EnginePy::Close,
             pybind11::arg("transportIndex") = -1);
}
