/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * gluePyBind11.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ADIOSPy.h"
#include "EnginePy.h"
#include "FilePy.h"
#include "IOPy.h"

#include "adios2/ADIOSTypes.h"

#include <stdexcept>

#include <adios2.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi4py/mpi4py.h>
#endif

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

adios2::FilePy Open(const std::string &name, const std::string mode,
                    pybind11::object &object, const std::string engineType,
                    const adios2::Params &parameters,
                    const adios2::vParams &transportParameters)
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
    return adios2::FilePy(name, mode, *mpiCommPtr, engineType, parameters,
                          transportParameters);
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

adios2::FilePy Open(const std::string &name, const std::string mode,
                    const std::string engineType,
                    const adios2::Params &parameters,
                    const adios2::vParams &transportParameters)
{
    return adios2::FilePy(name, mode, engineType, parameters,
                          transportParameters);
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

    m.doc() = "ADIOS2 Python bindings powered by pybind11";

    m.attr("DebugON") = true;
    m.attr("DebugOFF") = false;
    m.attr("ConstantDims") = true;
    m.attr("VariableDims") = false;
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
          pybind11::arg("object"), pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSPyInitConfig, "Function that creates an ADIOS class "
                                       "object using a config file",
          pybind11::arg("configFile") = "", pybind11::arg("object"),
          pybind11::arg("debugMode") = true);

    m.def("open", &Open, "High-level MPI file object open",
          pybind11::arg("name"), pybind11::arg("mode"), pybind11::arg("object"),
          pybind11::arg("engineType") = "BPFile",
          pybind11::arg("parameters") = adios2::Params(),
          pybind11::arg("transportParameters") = adios2::vParams());
#else
    m.def("ADIOS", &ADIOSPyInit,
          "Function that creates an ADIOS class object in non MPI mode",
          pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSPyInitConfig,
          "Function that creates an ADIOS class "
          "object using a config file in non MPI mode",
          pybind11::arg("configFile") = "", pybind11::arg("debugMode") = true);

    m.def("open", &Open, "High-level non-MPI file object open",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("engineType") = "BPFile",
          pybind11::arg("parameters") = adios2::Params(),
          pybind11::arg("transportParameters") = adios2::vParams());
#endif

    pybind11::class_<adios2::ADIOSPy>(m, "ADIOSPy")
        .def("DeclareIO", &adios2::ADIOSPy::DeclareIO)
        .def("AtIO", &adios2::ADIOSPy::AtIO)
        .def("FlushAll", &adios2::ADIOSPy::FlushAll);

    pybind11::class_<adios2::VariableBase>(m, "Variable")
        .def("SetShape", &adios2::VariableBase::SetShape)
        .def("SetSelection", &adios2::VariableBase::SetSelection)
        .def("SetStepSelection", &adios2::VariableBase::SetStepSelection)
        .def("SelectionSize", &adios2::VariableBase::SelectionSize);

    pybind11::class_<adios2::AttributeBase>(m, "Attribute");

    pybind11::class_<adios2::IOPy>(m, "IOPy")
        .def("SetEngine", &adios2::IOPy::SetEngine)
        .def("SetParameters", &adios2::IOPy::SetParameters,
             pybind11::arg("parameters") = adios2::Params())
        .def("SetParameters", &adios2::IOPy::SetParameters)
        .def("GetParameters", &adios2::IOPy::GetParameters,
             pybind11::return_value_policy::reference_internal)
        .def("AddTransport", &adios2::IOPy::AddTransport)

        .def("DefineVariable",
             (adios2::VariableBase *
              (adios2::IOPy::*)(const std::string &, const adios2::Dims &,
                                const adios2::Dims &, const adios2::Dims &,
                                const bool, pybind11::array &)) &
                 adios2::IOPy::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("shape") = adios2::Dims{},
             pybind11::arg("start") = adios2::Dims{},
             pybind11::arg("count") = adios2::Dims{},
             pybind11::arg("isConstantDims") = false,
             pybind11::arg("array") = pybind11::array())

        .def("DefineVariable",
             (adios2::VariableBase *
              (adios2::IOPy::*)(const std::string &, std::string &)) &
                 adios2::IOPy::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("stringValue"))

        .def("InquireVariable", &adios2::IOPy::InquireVariable,
             pybind11::return_value_policy::reference_internal)

        .def("DefineAttribute",
             (adios2::AttributeBase *
              (adios2::IOPy::*)(const std::string &, pybind11::array &)) &
                 adios2::IOPy::DefineAttribute,
             pybind11::return_value_policy::reference_internal)
        .def("DefineAttribute",
             (adios2::AttributeBase *
              (adios2::IOPy::*)(const std::string &,
                                const std::vector<std::string> &)) &
                 adios2::IOPy::DefineAttribute,
             pybind11::return_value_policy::reference_internal)
        .def("Open", (adios2::EnginePy (adios2::IOPy::*)(const std::string &,
                                                         const int)) &
                         adios2::IOPy::Open)
        .def("FlushAll", &adios2::IOPy::FlushAll);

    pybind11::class_<adios2::EnginePy>(m, "EnginePy")
        .def("BeginStep", &adios2::EnginePy::BeginStep,
             pybind11::arg("mode") = adios2::StepMode::NextAvailable,
             pybind11::arg("timeoutSeconds") = 0.f)
        .def("PutSync", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                    const pybind11::array &)) &
                            adios2::EnginePy::PutSync)
        .def("PutSync", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                    const std::string &)) &
                            adios2::EnginePy::PutSync)
        .def("PutDeferred",
             (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                         const pybind11::array &)) &
                 adios2::EnginePy::PutDeferred)
        .def("PutDeferred", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                        const std::string &)) &
                                adios2::EnginePy::PutDeferred)
        .def("PerformPuts", &adios2::EnginePy::PerformPuts)
        .def("GetSync", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                    pybind11::array &)) &
                            adios2::EnginePy::GetSync)
        .def("GetSync", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                    std::string &)) &
                            adios2::EnginePy::GetSync)
        .def("GetDeferred", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                        pybind11::array &)) &
                                adios2::EnginePy::GetDeferred)
        .def("GetDeferred", (void (adios2::EnginePy::*)(adios2::VariableBase *,
                                                        std::string &)) &
                                adios2::EnginePy::GetDeferred)
        .def("PerformGets", &adios2::EnginePy::PerformGets)
        .def("EndStep", &adios2::EnginePy::EndStep)
        .def("WriteStep", &adios2::EnginePy::WriteStep)
        .def("Flush", &adios2::EnginePy::Flush)
        .def("Close", &adios2::EnginePy::Close,
             pybind11::arg("transportIndex") = -1);

    pybind11::class_<adios2::FilePy>(m, "FilePy")
        .def("eof", &adios2::FilePy::eof)
        .def("__repr__",
             [](const adios2::FilePy &a) {
                 return "<adios2.file named '" + a.m_Name + "' and mode '" +
                        a.m_Mode + "'>";
             })
        .def_property_readonly("closed", &adios2::FilePy::IsClosed)
        .def("write", (void (adios2::FilePy::*)(
                          const std::string &, const pybind11::array &,
                          const adios2::Dims &, const adios2::Dims &,
                          const adios2::Dims &, const bool endl)) &
                          adios2::FilePy::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("shape"), pybind11::arg("start"),
             pybind11::arg("count"), pybind11::arg("endl") = false)

        .def("write", (void (adios2::FilePy::*)(const std::string &,
                                                const pybind11::array &,
                                                const bool endl)) &
                          adios2::FilePy::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("endl") = false)

        .def("write",
             (void (adios2::FilePy::*)(const std::string &, const std::string &,
                                       const bool endl)) &
                 adios2::FilePy::Write,
             pybind11::arg("name"), pybind11::arg("stringValue"),
             pybind11::arg("endl") = false)

        .def("readstring", (std::string (adios2::FilePy::*)(const std::string &,
                                                            const bool)) &
                               adios2::FilePy::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("endl") = false)

        .def("readstring", (std::string (adios2::FilePy::*)(const std::string &,
                                                            const size_t)) &
                               adios2::FilePy::ReadString,
             pybind11::return_value_policy::take_ownership)

        .def("read", (pybind11::array (adios2::FilePy::*)(const std::string &,
                                                          const bool endl)) &
                         adios2::FilePy::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("endl") = false)

        .def("read", (pybind11::array (adios2::FilePy::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &, const bool)) &
                         adios2::FilePy::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("selectionStart"),
             pybind11::arg("selectionCount"), pybind11::arg("endl") = false)

        .def("read", (pybind11::array (adios2::FilePy::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &, const size_t, const size_t)) &
                         adios2::FilePy::Read,
             pybind11::return_value_policy::take_ownership)

        .def("close", &adios2::FilePy::Close);
}
