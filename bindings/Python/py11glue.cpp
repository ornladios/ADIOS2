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
                        pybind11::object &object, const std::string engineType)
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
    return adios2::py11::File(name, mode, *mpiCommPtr, engineType);
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
                        const std::string engineType)
{
    return adios2::py11::File(name, mode, engineType);
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
        .value("Deferred", adios2::Mode::Deferred)
        .value("Sync", adios2::Mode::Sync)
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
    m.def("ADIOS", &ADIOSInit,
          "adios2 module starting point, creates an ADIOS class object",
          pybind11::arg("object"), pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSInitConfig, "adios2 module starting point, creates an "
                                     "ADIOS class object including a runtime "
                                     "config file",
          pybind11::arg("configFile") = "", pybind11::arg("object"),
          pybind11::arg("debugMode") = true);

    m.def("open", &Open, "High-level API, file object open",
          pybind11::arg("name"), pybind11::arg("mode"), pybind11::arg("object"),
          pybind11::arg("engineType") = "BPFile");

    m.def("open", &OpenConfig,
          "High-level API, file object open with a runtime config file",
          pybind11::arg("name"), pybind11::arg("mode"), pybind11::arg("object"),
          pybind11::arg("configFile"), pybind11::arg("ioInConfigFile"));

#else
    m.def("ADIOS", &ADIOSInit,
          "adios2 module starting point NON MPI, creates an ADIOS class object",
          pybind11::arg("debugMode") = true);

    m.def("ADIOS", &ADIOSInitConfig,
          "adios2 module starting point NON MPI, creates an "
          "ADIOS class object including a runtime config file",
          pybind11::arg("configFile") = "", pybind11::arg("debugMode") = true);

    m.def("open", &Open, "High-level API, file object open",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("engineType") = "BPFile");

    m.def("open", &OpenConfig,
          "High-level API, file object open with a runtime config file",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("configFile"), pybind11::arg("ioInConfigFile"));
#endif

    pybind11::class_<adios2::py11::ADIOS>(m, "py11::ADIOS")
        .def("DeclareIO", &adios2::py11::ADIOS::DeclareIO,
             "spawn IO object component returning a IO object with a unique "
             "name, throws an exception if IO with the same name is declared "
             "twice")
        .def("AtIO", &adios2::py11::ADIOS::AtIO, "returns an IO object "
                                                 "previously defined IO object "
                                                 "with DeclareIO, throws "
                                                 "an exception if not found")
        .def("FlushAll", &adios2::py11::ADIOS::FlushAll,
             "flushes all engines in all spawned IO objects");

    pybind11::class_<adios2::core::VariableBase>(m, "Variable")
        .def("SetShape", &adios2::core::VariableBase::SetShape)
        .def("SetSelection", &adios2::core::VariableBase::SetSelection)
        .def("SetStepSelection", &adios2::core::VariableBase::SetStepSelection)
        .def("SelectionSize", &adios2::core::VariableBase::SelectionSize);

    pybind11::class_<adios2::core::AttributeBase>(m, "Attribute");

    pybind11::class_<adios2::py11::IO>(m, "py11::IO")
        .def("SetEngine", &adios2::py11::IO::SetEngine)
        .def("SetParameters", &adios2::py11::IO::SetParameters,
             pybind11::arg("parameters") = adios2::Params())
        .def("SetParameter", &adios2::py11::IO::SetParameter)
        .def("Parameters", &adios2::py11::IO::Parameters)
        .def("AddTransport", &adios2::py11::IO::AddTransport,
             pybind11::arg("type"),
             pybind11::arg("parameters") = adios2::Params())

        .def("DefineVariable",
             (adios2::core::VariableBase *
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
             (adios2::core::VariableBase *
              (adios2::py11::IO::*)(const std::string &, std::string &)) &
                 adios2::py11::IO::DefineVariable,
             pybind11::return_value_policy::reference_internal,
             pybind11::arg("name"), pybind11::arg("stringValue"))

        .def("InquireVariable", &adios2::py11::IO::InquireVariable,
             pybind11::return_value_policy::reference_internal)

        .def("InquireAttribute", &adios2::py11::IO::InquireAttribute,
             pybind11::return_value_policy::reference_internal)

        .def("DefineAttribute",
             (adios2::core::AttributeBase *
              (adios2::py11::IO::*)(const std::string &, pybind11::array &)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::return_value_policy::reference_internal)
        .def("DefineAttribute",
             (adios2::core::AttributeBase *
              (adios2::py11::IO::*)(const std::string &,
                                    const std::vector<std::string> &)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::return_value_policy::reference_internal)
        .def("Open", (adios2::py11::Engine (adios2::py11::IO::*)(
                         const std::string &, const int)) &
                         adios2::py11::IO::Open)
        .def("AvailableVariables", &adios2::py11::IO::AvailableVariables)
        .def("AvailableAttributes", &adios2::py11::IO::AvailableAttributes)
        .def("FlushAll", &adios2::py11::IO::FlushAll)
        .def("EngineType", &adios2::py11::IO::EngineType)
        .def("LockDefinitions", &adios2::py11::IO::LockDefinitions);

    pybind11::class_<adios2::py11::Engine>(m, "py11::Engine")
        .def("BeginStep", &adios2::py11::Engine::BeginStep,
             pybind11::arg("mode") = adios2::StepMode::NextAvailable,
             pybind11::arg("timeoutSeconds") = 0.f)

        .def("Put", (void (adios2::py11::Engine::*)(
                        adios2::core::VariableBase *, const pybind11::array &,
                        const adios2::Mode launch)) &
                        adios2::py11::Engine::Put,
             pybind11::arg("variable"), pybind11::arg("array"),
             pybind11::arg("launch") = adios2::Mode::Deferred)
        .def("Put", (void (adios2::py11::Engine::*)(
                        adios2::core::VariableBase *, const std::string &)) &
                        adios2::py11::Engine::Put)

        .def("PerformPuts", &adios2::py11::Engine::PerformPuts)

        .def("Get", (void (adios2::py11::Engine::*)(
                        adios2::core::VariableBase *, pybind11::array &,
                        const adios2::Mode launch)) &
                        adios2::py11::Engine::Get,
             pybind11::arg("variable"), pybind11::arg("array"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("Get", (void (adios2::py11::Engine::*)(
                        adios2::core::VariableBase *, std::string &,
                        const adios2::Mode launch)) &
                        adios2::py11::Engine::Get,
             pybind11::arg("variable"), pybind11::arg("string"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("PerformGets", &adios2::py11::Engine::PerformGets)

        .def("EndStep", &adios2::py11::Engine::EndStep)
        .def("Flush", &adios2::py11::Engine::Flush)
        .def("CurrentStep", &adios2::py11::Engine::CurrentStep)
        .def("Name", &adios2::py11::Engine::Name)
        .def("Type", &adios2::py11::Engine::Type)
        .def("Close", &adios2::py11::Engine::Close,
             pybind11::arg("transportIndex") = -1);

    pybind11::class_<adios2::py11::File>(m, "py11::File")
        .def("__repr__",
             [](const adios2::py11::File &stream) {
                 return "<adios2.file named '" + stream.m_Name +
                        "' and mode '" + stream.m_Mode + "'>";
             })

        // enter and exit are defined for the with-as operator in Python
        .def("__enter__",
             [](const adios2::py11::File &stream) { return stream; })
        .def("__exit__",
             [](adios2::py11::File &stream, pybind11::args) { stream.Close(); })

        .def("__iter__", [](adios2::py11::File &stream) { return stream; },
             pybind11::keep_alive<0, 1>())
        .def("__next__",
             [](adios2::py11::File &stream) {
                 if (!stream.GetStep())
                 {
                     throw pybind11::stop_iteration();
                 }
                 return stream;
             })

        .def("setparameter", &adios2::py11::File::SetParameter,
             pybind11::arg("key"), pybind11::arg("value"))

        .def("setparameters", &adios2::py11::File::SetParameters,
             pybind11::arg("kwargs"))

        .def("addtransport", &adios2::py11::File::AddTransport,
             pybind11::return_value_policy::move, pybind11::arg("type"),
             pybind11::arg("kwargs") = adios2::Params())

        .def("availablevariables", &adios2::py11::File::AvailableVariables,
             pybind11::return_value_policy::move)

        .def("availableattributes", &adios2::py11::File::AvailableAttributes,
             pybind11::return_value_policy::move)

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
             pybind11::arg("name"), pybind11::arg("string"),
             pybind11::arg("endl") = false)

        .def("readstring",
             (std::string (adios2::py11::File::*)(const std::string &)) &
                 adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"))

        .def("readstring", (std::string (adios2::py11::File::*)(
                               const std::string &, const size_t)) &
                               adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership)

        .def("read",
             (pybind11::array (adios2::py11::File::*)(const std::string &)) &
                 adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"))

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("start"),
             pybind11::arg("count"))

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &, const size_t, const size_t)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("start"),
             pybind11::arg("count"), pybind11::arg("stepstart"),
             pybind11::arg("stepcount"))

        .def("close", &adios2::py11::File::Close)

        .def("getstep", &adios2::py11::File::GetStep)

        .def("currentstep", &adios2::py11::File::CurrentStep);
}
