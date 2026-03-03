/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11glue.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/complex.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <complex>
#include <sstream>
#include <stdexcept>

#include <adios2.h>

#if ADIOS2_USE_MPI
#include <mpi4py/mpi4py.h>
#endif

#include "nbADIOS.h"
#include "nbAttribute.h"
#include "nbEngine.h"
#include "nbIO.h"
#include "nbOperator.h"
#include "nbQuery.h"
#include "nbVariable.h"
#include "nbVariableDerived.h"

namespace nb = nanobind;

#if ADIOS2_USE_MPI

namespace nanobind
{
namespace detail
{
template <>
struct type_caster<adios2::py11::MPI4PY_Comm>
{
public:
    NB_TYPE_CASTER(adios2::py11::MPI4PY_Comm, const_name("MPI4PY_Comm"))

    bool from_python(handle src, uint8_t /*flags*/, cleanup_list *) noexcept
    {
        // Import mpi4py if it does not exist.
        if (!PyMPIComm_Get)
        {
            if (import_mpi4py() < 0)
            {
                return false;
            }
        }
        // If src is not actually a MPI4PY communicator, the next
        // call returns nullptr, and we return false to indicate the conversion
        // failed.
        MPI_Comm *mpiCommPtr = PyMPIComm_Get(src.ptr());
        if (mpiCommPtr == nullptr)
        {
            return false;
        }
        value.comm = *mpiCommPtr;
        return true;
    }

    static handle from_cpp(const adios2::py11::MPI4PY_Comm &, rv_policy, cleanup_list *) noexcept
    {
        // C++ → Python conversion not needed for this type
        return none().release();
    }
};
} // namespace detail
} // namespace nanobind

#endif

NB_MODULE(ADIOS2_PYTHON_MODULE_NAME, m)
{
    m.attr("ConstantDims") = true;
    m.attr("VariableDims") = false;
    m.attr("LocalValueDim") = adios2::LocalValueDim;
    m.attr("GlobalValue") = false;
    m.attr("LocalValue") = true;

    m.attr("__version__") = ADIOS2_VERSION_STR;
#if defined(ADIOS2_HAVE_MPI)
    m.attr("is_built_with_mpi") = true;
#else
    m.attr("is_built_with_mpi") = false;
#endif

    m.attr("L2_norm") = adios2::L2_norm;
    m.attr("Linf_norm") = adios2::Linf_norm;

    // enum classes
    nb::enum_<adios2::Mode>(m, "Mode")
        .value("Write", adios2::Mode::Write)
        .value("Read", adios2::Mode::Read)
        .value("ReadRandomAccess", adios2::Mode::ReadRandomAccess)
        .value("Append", adios2::Mode::Append)
        .value("Deferred", adios2::Mode::Deferred)
        .value("Sync", adios2::Mode::Sync)
        .export_values();

    nb::enum_<adios2::ShapeID>(m, "ShapeID")
        .value("Unknown", adios2::ShapeID::Unknown)
        .value("GlobalValue", adios2::ShapeID::GlobalValue)
        .value("GlobalArray", adios2::ShapeID::GlobalArray)
        .value("LocalValue", adios2::ShapeID::LocalValue)
        .value("LocalArray", adios2::ShapeID::LocalArray)
        .export_values();

    nb::enum_<adios2::StepMode>(m, "StepMode")
        .value("Append", adios2::StepMode::Append)
        .value("Update", adios2::StepMode::Update)
        .value("Read", adios2::StepMode::Read)
        .export_values();

    nb::enum_<adios2::StepStatus>(m, "StepStatus")
        .value("OK", adios2::StepStatus::OK)
        .value("NotReady", adios2::StepStatus::NotReady)
        .value("EndOfStream", adios2::StepStatus::EndOfStream)
        .value("OtherError", adios2::StepStatus::OtherError)
        .export_values();

    nb::enum_<adios2::DerivedVarType>(m, "DerivedVarType")
        .value("StatsOnly", adios2::DerivedVarType::StatsOnly)
        .value("ExpressionString", adios2::DerivedVarType::ExpressionString)
        .value("StoreData", adios2::DerivedVarType::StoreData)
        .export_values();

#ifdef ADIOS2_HAVE_GPU_SUPPORT
    nb::enum_<adios2::MemorySpace>(m, "MemorySpace")
        .value("Host", adios2::MemorySpace::Host)
        .value("GPU", adios2::MemorySpace::GPU);
#endif

    nb::class_<adios2::Accuracy>(m, "Accuracy")
        .def(nb::init<double, double, bool>())
        .def_rw("error", &adios2::Accuracy::error)
        .def_rw("norm", &adios2::Accuracy::norm)
        .def_rw("relative", &adios2::Accuracy::relative)

        .def("__repr__", [](const adios2::Accuracy &self) {
            std::ostringstream _stream;
            _stream << "(" << self.error << ", " << self.norm << ", " << self.relative << ")";
            return _stream.str();
        });

    nb::class_<adios2::py11::ADIOS>(m, "ADIOS")
        .def("__bool__",
             [](const adios2::py11::ADIOS &adios) {
                 const bool opBool = adios ? true : false;
                 return opBool;
             })
        .def(nb::init(), "adios2 module starting point "
                         "non-MPI, constructs an ADIOS class "
                         "object")
        .def(nb::init<const std::string &>(),
             "adios2 module starting point non-MPI, constructs an ADIOS class "
             "object",
             nb::arg("configFile"))
#if ADIOS2_USE_MPI
        .def(nb::init<const adios2::py11::MPI4PY_Comm>(),
             "adios2 module starting point, constructs an ADIOS class object", nb::arg("comm"))
        .def(nb::init<const std::string &, const adios2::py11::MPI4PY_Comm>(),
             "adios2 module starting point, constructs an ADIOS class object",
             nb::arg("configFile"), nb::arg("comm"))
#endif
        .def("DeclareIO", &adios2::py11::ADIOS::DeclareIO,
             "spawn IO object component returning a IO object with a unique "
             "name, throws an exception if IO with the same name is declared "
             "twice")
        .def("AtIO", &adios2::py11::ADIOS::AtIO,
             "returns an IO object "
             "previously defined IO object "
             "with DeclareIO, throws "
             "an exception if not found")
        .def("DefineOperator", &adios2::py11::ADIOS::DefineOperator)
        .def("InquireOperator", &adios2::py11::ADIOS::InquireOperator)
        .def("FlushAll", &adios2::py11::ADIOS::FlushAll,
             "flushes all engines in all spawned IO objects")
        .def("RemoveIO", &adios2::py11::ADIOS::RemoveIO,
             "DANGER ZONE: remove a particular IO by name, creates dangling "
             "objects to parameters, variable, attributes, engines created "
             "with removed IO")
        .def("RemoveAllIOs", &adios2::py11::ADIOS::RemoveAllIOs,
             "DANGER ZONE: remove all IOs in current ADIOS object, creates "
             "dangling objects to parameters, variable, attributes, engines "
             "created with removed IO");

    nb::class_<adios2::py11::IO>(m, "IO")
        .def("__bool__",
             [](const adios2::py11::IO &io) {
                 const bool opBool = io ? true : false;
                 return opBool;
             })
        .def("SetEngine", &adios2::py11::IO::SetEngine)
        .def("SetParameters", &adios2::py11::IO::SetParameters,
             nb::arg("parameters") = adios2::Params())
        .def("SetParameter", &adios2::py11::IO::SetParameter)
        .def("Parameters", &adios2::py11::IO::Parameters)
        .def("AddTransport", &adios2::py11::IO::AddTransport, nb::arg("type"),
             nb::arg("parameters") = adios2::Params())

        .def("DefineVariable",
             (adios2::py11::Variable(adios2::py11::IO::*)(
                 const std::string &, const nb::ndarray<nb::numpy> &, const adios2::Dims &,
                 const adios2::Dims &, const adios2::Dims &, const bool)) &
                 adios2::py11::IO::DefineVariable,
             nb::arg("name"), nb::arg("array"), nb::arg("shape") = adios2::Dims(),
             nb::arg("start") = adios2::Dims(), nb::arg("count") = adios2::Dims(),
             nb::arg("isConstantDims") = false)

        .def("DefineVariable",
             (adios2::py11::Variable(adios2::py11::IO::*)(
                 const std::string &, const nb::object &, const adios2::Dims &,
                 const adios2::Dims &, const adios2::Dims &, const bool)) &
                 adios2::py11::IO::DefineVariable,
             nb::arg("name"), nb::arg("value"), nb::arg("shape") = adios2::Dims(),
             nb::arg("start") = adios2::Dims(), nb::arg("count") = adios2::Dims(),
             nb::arg("isConstantDims") = false)

        .def("DefineVariable",
             (adios2::py11::Variable(adios2::py11::IO::*)(const std::string &)) &
                 adios2::py11::IO::DefineVariable,
             nb::arg("name"))

        .def("DefineDerivedVariable",
             (adios2::py11::VariableDerived(adios2::py11::IO::*)(
                 const std::string &, const std::string &, const adios2::DerivedVarType)) &
                 adios2::py11::IO::DefineDerivedVariable,
             nb::arg("name"), nb::arg("expression"),
             nb::arg("vartype") = adios2::DerivedVarType::StatsOnly)

        .def("InquireVariable", &adios2::py11::IO::InquireVariable)

        .def("InquireAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(const std::string &, const std::string &,
                                                           const std::string)) &
                 adios2::py11::IO::InquireAttribute,
             nb::arg("name"), nb::arg("variable_name") = "", nb::arg("separator") = "/")

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const nb::ndarray<nb::numpy> &, const std::string &,
                 const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             nb::arg("name"), nb::arg("array"), nb::arg("variable_name") = "",
             nb::arg("separator") = "/")

        .def(
            "DefineAttribute",
            (adios2::py11::Attribute(adios2::py11::IO::*)(const std::string &, const std::string &,
                                                          const std::string &, const std::string)) &
                adios2::py11::IO::DefineAttribute,
            nb::arg("name"), nb::arg("stringValue"), nb::arg("variable_name") = "",
            nb::arg("separator") = "/")

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const std::vector<std::string> &, const std::string &,
                 const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             nb::arg("name"), nb::arg("strings"), nb::arg("variable_name") = "",
             nb::arg("separator") = "/")

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const std::vector<int> &, const std::string &,
                 const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             nb::arg("name"), nb::arg("ints"), nb::arg("variable_name") = "",
             nb::arg("separator") = "/")

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const std::vector<double> &, const std::string &,
                 const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             nb::arg("name"), nb::arg("doubles"), nb::arg("variable_name") = "",
             nb::arg("separator") = "/")

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const std::vector<std::complex<double>> &,
                 const std::string &, const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             nb::arg("name"), nb::arg("complexes"), nb::arg("variable_name") = "",
             nb::arg("separator") = "/")

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const nb::object &, const std::string &, const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             nb::arg("name"), nb::arg("value"), nb::arg("variable_name") = "",
             nb::arg("separator") = "/")

        .def("Open",
             (adios2::py11::Engine(adios2::py11::IO::*)(const std::string &, const adios2::Mode)) &
                 adios2::py11::IO::Open)

        .def("Open",
             (adios2::py11::Engine(adios2::py11::IO::*)(const std::string &, const nb::bytes &)) &
                 adios2::py11::IO::Open)
#if ADIOS2_USE_MPI
        .def("Open", (adios2::py11::Engine(adios2::py11::IO::*)(
                         const std::string &, const adios2::Mode, adios2::py11::MPI4PY_Comm)) &
                         adios2::py11::IO::Open)
#endif
        .def("AvailableAttributes", &adios2::py11::IO::AvailableAttributes, nb::arg("varname") = "",
             nb::arg("separator") = "/")

        .def("AvailableVariables", &adios2::py11::IO::AvailableVariables)
        .def("FlushAll", &adios2::py11::IO::FlushAll)
        .def("EngineType", &adios2::py11::IO::EngineType)
        .def("RemoveVariable", &adios2::py11::IO::RemoveVariable)
        .def("RemoveAllVariables", &adios2::py11::IO::RemoveAllVariables)
        .def("RemoveAttribute", &adios2::py11::IO::RemoveAttribute)
        .def("RemoveAllAttributes", &adios2::py11::IO::RemoveAllAttributes);

    nb::class_<adios2::py11::Query>(m, "Query")
        .def("__bool__",
             [](const adios2::py11::Query &query) {
                 const bool opBool = query ? true : false;
                 return opBool;
             })
        .def(nb::init<const std::string &, const adios2::py11::Engine &>(),
             "adios2 query construction, a xml query File and a read engine", nb::arg("queryFile"),
             nb::arg("reader") = true)

        .def("GetResult", &adios2::py11::Query::GetResult)
        .def("GetBlockIDs", &adios2::py11::Query::GetBlockIDs);

    nb::class_<adios2::py11::Operator>(m, "Operator")
        .def("__bool__",
             [](const adios2::py11::Operator &op) {
                 const bool opBool = op ? true : false;
                 return opBool;
             })
        .def("Type", &adios2::py11::Operator::Type)
        .def("SetParameter", &adios2::py11::Operator::SetParameter)
        .def("Parameters", &adios2::py11::Operator::Parameters);

    nb::class_<adios2::py11::Variable>(m, "Variable")
        .def("__bool__",
             [](const adios2::py11::Variable &variable) {
                 const bool opBool = variable ? true : false;
                 return opBool;
             })
        .def("SetShape", &adios2::py11::Variable::SetShape)
        .def("StoreStatsOnly", &adios2::py11::Variable::StoreStatsOnly)
        .def("SetBlockSelection", &adios2::py11::Variable::SetBlockSelection)
        .def("SetSelection", &adios2::py11::Variable::SetSelection)
        .def("SetStepSelection", &adios2::py11::Variable::SetStepSelection)
        .def("SetAccuracy", &adios2::py11::Variable::SetAccuracy)
        .def("GetAccuracy", &adios2::py11::Variable::GetAccuracy)
        .def("GetAccuracyRequested", &adios2::py11::Variable::GetAccuracyRequested)
        .def("SelectionSize", &adios2::py11::Variable::SelectionSize)
        .def("Name", &adios2::py11::Variable::Name)
        .def("Type", &adios2::py11::Variable::Type)
        .def("Sizeof", &adios2::py11::Variable::Sizeof)
        .def("ShapeID", &adios2::py11::Variable::ShapeID)
        .def("Shape",
             (adios2::Dims(adios2::py11::Variable::*)(const size_t) const) &
                 adios2::py11::Variable::Shape,
             nb::arg("step") = adios2::EngineCurrentStep)
#ifdef ADIOS2_HAVE_GPU_SUPPORT
        .def("Shape",
             (adios2::Dims(adios2::py11::Variable::*)(const adios2::MemorySpace, const size_t)
                  const) &
                 adios2::py11::Variable::Shape,
             nb::arg("memSpace"), nb::arg("step") = adios2::EngineCurrentStep)
        .def("SetMemorySpace",
             (void(adios2::py11::Variable::*)(const adios2::MemorySpace)) &
                 adios2::py11::Variable::SetMemorySpace,
             nb::arg("memSpace"))
#endif
        .def("Start", &adios2::py11::Variable::Start)
        .def("Count", &adios2::py11::Variable::Count)
        .def("Steps", &adios2::py11::Variable::Steps)
        .def("StepsStart", &adios2::py11::Variable::StepsStart)
        .def("BlockID", &adios2::py11::Variable::BlockID)
        .def("SingleValue", &adios2::py11::Variable::SingleValue)
        .def("AddOperation", (size_t(adios2::py11::Variable::*)(const adios2::py11::Operator,
                                                                const adios2::Params &)) &
                                 adios2::py11::Variable::AddOperation)
        .def("AddOperation",
             (size_t(adios2::py11::Variable::*)(const std::string &, const adios2::Params &)) &
                 adios2::py11::Variable::AddOperation)
        .def("Operations", &adios2::py11::Variable::Operations)
        .def("RemoveOperations", &adios2::py11::Variable::RemoveOperations);

    nb::class_<adios2::py11::VariableDerived>(m, "VariableDerived")
        .def("__bool__",
             [](const adios2::py11::VariableDerived &vd) {
                 const bool opBool = vd ? true : false;
                 return opBool;
             })
        .def("Name", &adios2::py11::VariableDerived::Name)
        .def("Type", &adios2::py11::VariableDerived::Type);

    nb::class_<adios2::py11::Attribute>(m, "Attribute")
        .def("__bool__",
             [](const adios2::py11::Attribute &attribute) {
                 const bool opBool = attribute ? true : false;
                 return opBool;
             })
        .def("Name", &adios2::py11::Attribute::Name)
        .def("Type", &adios2::py11::Attribute::Type)
        .def("DataString", &adios2::py11::Attribute::DataString)
        .def("Data", &adios2::py11::Attribute::Data)
        .def("SingleValue", &adios2::py11::Attribute::SingleValue);

    nb::class_<adios2::py11::Engine>(m, "Engine")
        .def("__bool__",
             [](const adios2::py11::Engine &engine) {
                 const bool opBool = engine ? true : false;
                 return opBool;
             })

        .def("GetMetadata", &adios2::py11::Engine::GetMetadata)

        .def("BeginStep",
             (adios2::StepStatus(adios2::py11::Engine::*)(const adios2::StepMode, const float)) &
                 adios2::py11::Engine::BeginStep,
             nb::arg("mode"), nb::arg("timeoutSeconds") = -1.f)

        .def("BeginStep",
             (adios2::StepStatus(adios2::py11::Engine::*)()) & adios2::py11::Engine::BeginStep)

        .def("Put",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable, const nb::ndarray<nb::numpy> &,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Put,
             nb::arg("variable"), nb::arg("array"), nb::arg("launch") = adios2::Mode::Deferred)

        .def("Put", (void(adios2::py11::Engine::*)(adios2::py11::Variable, const std::string &)) &
                        adios2::py11::Engine::Put)

        .def("Put",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable, const std::vector<int64_t> &,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Put,
             nb::arg("variable"), nb::arg("ints"), nb::arg("launch") = adios2::Mode::Sync)

        .def("Put",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable, const std::vector<double> &,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Put,
             nb::arg("variable"), nb::arg("floats"), nb::arg("launch") = adios2::Mode::Sync)

        .def("Put",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable variable, std::uintptr_t,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Put,
             nb::arg("variable"), nb::arg("pointer"), nb::arg("launch"))

        .def("Put",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable,
                                            const std::vector<std::complex<double>> &,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Put,
             nb::arg("variable"), nb::arg("complexes"), nb::arg("launch") = adios2::Mode::Sync)

        .def("PerformPuts", &adios2::py11::Engine::PerformPuts)

        .def("PerformDataWrite", &adios2::py11::Engine::PerformDataWrite)

        .def("Get",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable, nb::ndarray<nb::numpy> &,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Get,
             nb::arg("variable"), nb::arg("array"), nb::arg("launch") = adios2::Mode::Deferred)

        .def("Get",
             (std::string(adios2::py11::Engine::*)(adios2::py11::Variable,
                                                   const adios2::Mode launch)) &
                 adios2::py11::Engine::Get,
             nb::arg("variable"), nb::arg("launch") = adios2::Mode::Deferred)

        .def("Get",
             (void(adios2::py11::Engine::*)(adios2::py11::Variable variable, std::uintptr_t,
                                            const adios2::Mode launch)) &
                 adios2::py11::Engine::Get,
             nb::arg("variable"), nb::arg("pointer"), nb::arg("launch"))

        .def("PerformGets", &adios2::py11::Engine::PerformGets)

        .def("EndStep", &adios2::py11::Engine::EndStep)

        .def("BetweenStepPairs", &adios2::py11::Engine::BetweenStepPairs)

        .def("Flush", &adios2::py11::Engine::Flush)

        .def("Close", &adios2::py11::Engine::Close, nb::arg("transportIndex") = -1)

        .def("CurrentStep", &adios2::py11::Engine::CurrentStep)

        .def("Name", &adios2::py11::Engine::Name)

        .def("Type", &adios2::py11::Engine::Type)

        .def("Steps", &adios2::py11::Engine::Steps)

        .def("LockWriterDefinitions", &adios2::py11::Engine::LockWriterDefinitions)

        .def("LockReaderSelections", &adios2::py11::Engine::LockReaderSelections)

        .def("BlocksInfo", &adios2::py11::Engine::BlocksInfo);
}
