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

#include <stdexcept>

#include <adios2.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi4py/mpi4py.h>
#endif

#include "py11ADIOS.h"
#include "py11Engine.h"
#include "py11File.h"
#include "py11IO.h"

#ifdef ADIOS2_HAVE_MPI
adios2::py11::ADIOS ADIOSInitConfig(const std::string configFile,
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
    return adios2::py11::ADIOS(configFile, *mpiCommPtr, debugMode);
}

adios2::py11::ADIOS ADIOSInit(pybind11::object &object, const bool debugMode)
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
    return adios2::py11::ADIOS(*mpiCommPtr, debugMode);
}

adios2::py11::File Open(const std::string &name, const std::string mode,
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
    return adios2::py11::File(name, mode, *mpiCommPtr, engineType, parameters,
                              transportParameters);
}

adios2::py11::File OpenConfig(const std::string &name, const std::string mode,
                              pybind11::object &object,
                              const std::string configFile,
                              const std::string ioInConfigFile)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(object.ptr());

    if (import_mpi4py() < 0)
    {
        throw std::runtime_error("ERROR: could not import mpi4py "
                                 "communicator, in call to open\n");
    }

    if (mpiCommPtr == nullptr)
    {
        throw std::runtime_error("ERROR: mpi4py communicator is null, in call "
                                 "to ADIOS constructor\n");
    }
    return adios2::py11::File(name, mode, *mpiCommPtr, configFile,
                              ioInConfigFile);
}

#else
adios2::py11::ADIOS ADIOSInitConfig(const std::string configFile,
                                    const bool debugMode)
{
    return adios2::py11::ADIOS(configFile, debugMode);
}

adios2::py11::ADIOS ADIOSInit(const bool debugMode)
{
    return adios2::py11::ADIOS(debugMode);
}

adios2::py11::File Open(const std::string &name, const std::string mode,
                        const std::string engineType,
                        const adios2::Params &parameters,
                        const adios2::vParams &transportParameters)
{
    return adios2::py11::File(name, mode, engineType, parameters,
                              transportParameters);
}

adios2::py11::File OpenConfig(const std::string &name, const std::string mode,
                              const std::string configFile,
                              const std::string ioInConfigFile)
{
    return adios2::py11::File(name, mode, configFile, ioInConfigFile);
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
    m.def("ADIOS", &ADIOSInit, "Function that creates an ADIOS class object",
          pybind11::arg("object"), pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSInitConfig, "Function that creates an ADIOS class "
                                     "object using a config file",
          pybind11::arg("configFile") = "", pybind11::arg("object"),
          pybind11::arg("debugMode") = true);

    m.def("open", &Open, "High-level file object open", pybind11::arg("name"),
          pybind11::arg("mode"), pybind11::arg("object"),
          pybind11::arg("engineType") = "BPFile",
          pybind11::arg("parameters") = adios2::Params(),
          pybind11::arg("transportParameters") = adios2::vParams());

    m.def("open", &OpenConfig, "High-level file object open with config file",
          pybind11::arg("name"), pybind11::arg("mode"), pybind11::arg("object"),
          pybind11::arg("configFile"), pybind11::arg("ioInConfigFile"));

#else
    m.def("ADIOS", &ADIOSInit,
          "Function that creates an ADIOS class object in non MPI mode",
          pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSInitConfig,
          "Function that creates an ADIOS class "
          "object using a config file in non MPI mode",
          pybind11::arg("configFile") = "", pybind11::arg("debugMode") = true);

    m.def("open", &Open, "High-level file object open", pybind11::arg("name"),
          pybind11::arg("mode"), pybind11::arg("engineType") = "BPFile",
          pybind11::arg("parameters") = adios2::Params(),
          pybind11::arg("transportParameters") = adios2::vParams());

    m.def("open", &OpenConfig, "High-level file object open with config file",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("configFile"), pybind11::arg("ioInConfigFile"));
#endif

    pybind11::class_<adios2::py11::ADIOS>(m, "py11::ADIOS")
        .def("DeclareIO", &adios2::py11::ADIOS::DeclareIO)
        .def("AtIO", &adios2::py11::ADIOS::AtIO)
        .def("FlushAll", &adios2::py11::ADIOS::FlushAll);

    pybind11::class_<adios2::VariableBase>(m, "Variable")
        .def("SetShape", &adios2::VariableBase::SetShape)
        .def("SetSelection", &adios2::VariableBase::SetSelection)
        .def("SetStepSelection", &adios2::VariableBase::SetStepSelection)
        .def("SelectionSize", &adios2::VariableBase::SelectionSize);

    pybind11::class_<adios2::AttributeBase>(m, "Attribute");

    pybind11::class_<adios2::py11::IO>(m, "py11::IO")
        .def("SetEngine", &adios2::py11::IO::SetEngine)
        .def("SetParameters", &adios2::py11::IO::SetParameters,
             pybind11::arg("parameters") = adios2::Params())
        .def("SetParameters", &adios2::py11::IO::SetParameters)
        .def("GetParameters", &adios2::py11::IO::GetParameters,
             pybind11::return_value_policy::reference_internal)
        .def("AddTransport", &adios2::py11::IO::AddTransport)

        .def("DefineVariable",
             (adios2::VariableBase *
              (adios2::py11::IO::*)(const std::string &, const adios2::Dims &,
                                    const adios2::Dims &, const adios2::Dims &,
                                    const bool, pybind11::array &)) &
                 adios2::py11::IO::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("shape") = adios2::Dims{},
             pybind11::arg("start") = adios2::Dims{},
             pybind11::arg("count") = adios2::Dims{},
             pybind11::arg("isConstantDims") = false,
             pybind11::arg("array") = pybind11::array())

        .def("DefineVariable",
             (adios2::VariableBase *
              (adios2::py11::IO::*)(const std::string &, std::string &)) &
                 adios2::py11::IO::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("stringValue"))

        .def("InquireVariable", &adios2::py11::IO::InquireVariable,
             pybind11::return_value_policy::reference_internal)

        .def("DefineAttribute",
             (adios2::AttributeBase *
              (adios2::py11::IO::*)(const std::string &, pybind11::array &)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::return_value_policy::reference_internal)
        .def("DefineAttribute",
             (adios2::AttributeBase *
              (adios2::py11::IO::*)(const std::string &,
                                    const std::vector<std::string> &)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::return_value_policy::reference_internal)
        .def("Open", (adios2::py11::Engine (adios2::py11::IO::*)(
                         const std::string &, const int)) &
                         adios2::py11::IO::Open)
        .def("GetAvailableVariables", &adios2::py11::IO::GetAvailableVariables)
        .def("FlushAll", &adios2::py11::IO::FlushAll);

    pybind11::class_<adios2::py11::Engine>(m, "py11::Engine")
        .def("BeginStep", &adios2::py11::Engine::BeginStep,
             pybind11::arg("mode") = adios2::StepMode::NextAvailable,
             pybind11::arg("timeoutSeconds") = 0.f)
        .def("PutSync", (void (adios2::py11::Engine::*)(
                            adios2::VariableBase *, const pybind11::array &)) &
                            adios2::py11::Engine::PutSync)
        .def("PutSync", (void (adios2::py11::Engine::*)(adios2::VariableBase *,
                                                        const std::string &)) &
                            adios2::py11::Engine::PutSync)
        .def("PutDeferred",
             (void (adios2::py11::Engine::*)(adios2::VariableBase *,
                                             const pybind11::array &)) &
                 adios2::py11::Engine::PutDeferred)
        .def("PutDeferred", (void (adios2::py11::Engine::*)(
                                adios2::VariableBase *, const std::string &)) &
                                adios2::py11::Engine::PutDeferred)
        .def("PerformPuts", &adios2::py11::Engine::PerformPuts)
        .def("GetSync", (void (adios2::py11::Engine::*)(adios2::VariableBase *,
                                                        pybind11::array &)) &
                            adios2::py11::Engine::GetSync)
        .def("GetSync", (void (adios2::py11::Engine::*)(adios2::VariableBase *,
                                                        std::string &)) &
                            adios2::py11::Engine::GetSync)
        .def("GetDeferred", (void (adios2::py11::Engine::*)(
                                adios2::VariableBase *, pybind11::array &)) &
                                adios2::py11::Engine::GetDeferred)
        .def("GetDeferred", (void (adios2::py11::Engine::*)(
                                adios2::VariableBase *, std::string &)) &
                                adios2::py11::Engine::GetDeferred)
        .def("PerformGets", &adios2::py11::Engine::PerformGets)
        .def("EndStep", &adios2::py11::Engine::EndStep)
        .def("WriteStep", &adios2::py11::Engine::WriteStep)
        .def("Flush", &adios2::py11::Engine::Flush)
        .def("CurrentStep", &adios2::py11::Engine::CurrentStep)
        .def("Close", &adios2::py11::Engine::Close,
             pybind11::arg("transportIndex") = -1);

    pybind11::class_<adios2::py11::File>(m, "py11::File")
        .def("eof", &adios2::py11::File::eof)
        .def("__repr__",
             [](const adios2::py11::File &a) {
                 return "<adios2.file named '" + a.m_Name + "' and mode '" +
                        a.m_Mode + "'>";
             })
        .def_property_readonly("closed", &adios2::py11::File::IsClosed)

        .def("available_variables", &adios2::py11::File::GetAvailableVariables)

        .def("write", (void (adios2::py11::File::*)(
                          const std::string &, const pybind11::array &,
                          const adios2::Dims &, const adios2::Dims &,
                          const adios2::Dims &, const bool endl)) &
                          adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("shape"), pybind11::arg("start"),
             pybind11::arg("count"), pybind11::arg("endl") = false)

        .def("write", (void (adios2::py11::File::*)(const std::string &,
                                                    const pybind11::array &,
                                                    const bool endl)) &
                          adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("endl") = false)

        .def("write",
             (void (adios2::py11::File::*)(
                 const std::string &, const std::string &, const bool endl)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("stringValue"),
             pybind11::arg("endl") = false)

        .def("readstring", (std::string (adios2::py11::File::*)(
                               const std::string &, const bool)) &
                               adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("endl") = false)

        .def("readstring", (std::string (adios2::py11::File::*)(
                               const std::string &, const size_t)) &
                               adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership)

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const bool endl)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("endl") = false)

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &, const bool)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("selectionStart"),
             pybind11::arg("selectionCount"), pybind11::arg("endl") = false)

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &, const size_t, const size_t)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership)

        .def("close", &adios2::py11::File::Close)
        .def("currentstep", &adios2::py11::File::CurrentStep);
}
