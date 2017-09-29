#include <adios2.h>

#include <pybind11/pybind11.h>

namespace adios2
{
template <typename T>
void GeneratePythonBindings(pybind11::module &m);

template <>
void GeneratePythonBindings<void>(pybind11::module &m)
{
    pybind11::enum_<ShapeID>(m, "ShapeID")
        .value("GlobalValue", ShapeID::GlobalValue)
        .value("GlobalArray", ShapeID::GlobalArray)
        .value("JoinedArray", ShapeID::JoinedArray)
        .value("LocalValue", ShapeID::LocalValue)
        .value("LocalArray", ShapeID::LocalArray)
        .export_values();

    pybind11::enum_<IOMode>(m, "IOMode")
        .value("Independent", IOMode::Independent)
        .value("Collective", IOMode::Collective)
        .export_values();

    pybind11::enum_<OpenMode>(m, "OpenMode")
        .value("Undefined", OpenMode::Undefined)
        .value("Write", OpenMode::Write)
        .value("Read", OpenMode::Read)
        .value("Append", OpenMode::Append)
        .value("ReadWrite", OpenMode::ReadWrite)
        .export_values();

    pybind11::enum_<ReadMultiplexPattern>(m, "ReadMultiplexPattern")
        .value("GlobalReaders", ReadMultiplexPattern::GlobalReaders)
        .value("RoundRobin", ReadMultiplexPattern::RoundRobin)
        .value("FirstInFirstOut", ReadMultiplexPattern::FirstInFirstOut)
        .value("OpenAllSteps", ReadMultiplexPattern::OpenAllSteps)
        .export_values();

    pybind11::enum_<ReadMode>(m, "ReadMode")
        .value("NonBlocking", ReadMode::Blocking)
        .value("Blocking", ReadMode::Blocking)
        .export_values();

    pybind11::enum_<AdvanceMode>(m, "AdvanceMode")
        .value("Append", AdvanceMode::Append)
        .value("Update", AdvanceMode::Update)
        .value("NextAvailable", AdvanceMode::NextAvailable)
        .value("LatestAvailable", AdvanceMode::LatestAvailable)
        .export_values();

    pybind11::enum_<AdvanceStatus>(m, "AdvanceStatus")
        .value("OK", AdvanceStatus::OK)
        .value("StepNotReady", AdvanceStatus::StepNotReady)
        .value("EndOfStream", AdvanceStatus::EndOfStream)
        .value("OtherError", AdvanceStatus::OtherError)
        .export_values();

    pybind11::enum_<SelectionType>(m, "SelectionType")
        .value("BoundingBox", SelectionType::BoundingBox)
        .value("Points", SelectionType::Points)
        .value("WriteBlock", SelectionType::WriteBlock)
        .value("Auto", SelectionType::Auto)
        .export_values();
}

} // end namespace adios2
