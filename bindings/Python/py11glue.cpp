/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * py11glue.cpp
 *
 *  Created on: Mar 16, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <sstream>
#include <stdexcept>

#include <adios2.h>

#if ADIOS2_USE_MPI
#include <mpi4py/mpi4py.h>
#endif

#include "py11ADIOS.h"
#include "py11Attribute.h"
#include "py11Engine.h"
#include "py11File.h"
#include "py11IO.h"
#include "py11Operator.h"
#include "py11Query.h"
#include "py11Variable.h"

#if ADIOS2_USE_MPI

namespace pybind11
{
namespace detail
{
template <>
struct type_caster<adios2::py11::MPI4PY_Comm>
{
public:
    /**
     * This macro establishes the name 'MPI4PY_Comm' in
     * function signatures and declares a local variable
     * 'value' of type MPI4PY_Comm
     */
    PYBIND11_TYPE_CASTER(adios2::py11::MPI4PY_Comm, _("MPI4PY_Comm"));

    /**
     * Conversion part 1 (Python->C++): convert a PyObject into a MPI4PY_Comm
     * instance or return false upon failure. The second argument
     * indicates whether implicit conversions should be applied.
     */
    bool load(handle src, bool)
    {
        // Import mpi4py if it does not exist.
        if (!PyMPIComm_Get)
        {
            if (import_mpi4py() < 0)
            {
                throw std::runtime_error(
                    "ERROR: mpi4py not loaded correctly\n"); /* Python 2.X */
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
};
} // namespace detail
} // namespace pybind11

#endif

#if ADIOS2_USE_MPI

adios2::py11::File OpenMPI(const std::string &name, const std::string mode,
                           adios2::py11::MPI4PY_Comm comm,
                           const std::string enginetype)
{
    return adios2::py11::File(name, mode, comm, enginetype);
}

adios2::py11::File OpenConfigMPI(const std::string &name,
                                 const std::string mode,
                                 adios2::py11::MPI4PY_Comm comm,
                                 const std::string &configfile,
                                 const std::string ioinconfigfile)
{
    return adios2::py11::File(name, mode, comm, configfile, ioinconfigfile);
}

#endif
adios2::py11::File Open(const std::string &name, const std::string mode,
                        const std::string enginetype)
{
    return adios2::py11::File(name, mode, enginetype);
}

adios2::py11::File OpenConfig(const std::string &name, const std::string mode,
                              const std::string configfile,
                              const std::string ioinconfigfile)
{
    return adios2::py11::File(name, mode, configfile, ioinconfigfile);
}

PYBIND11_MODULE(ADIOS2_PYTHON_MODULE_NAME, m)
{
    m.attr("ConstantDims") = true;
    m.attr("VariableDims") = false;
    m.attr("LocalValueDim") = adios2::LocalValueDim;
    m.attr("GlobalValue") = false;
    m.attr("LocalValue") = true;

    m.attr("__version__") = ADIOS2_VERSION_STR;

    // enum classes
    pybind11::enum_<adios2::Mode>(m, "Mode")
        .value("Write", adios2::Mode::Write)
        .value("Read", adios2::Mode::Read)
        .value("ReadRandomAccess", adios2::Mode::ReadRandomAccess)
        .value("Append", adios2::Mode::Append)
        .value("Deferred", adios2::Mode::Deferred)
        .value("Sync", adios2::Mode::Sync)
        .export_values();

    pybind11::enum_<adios2::ShapeID>(m, "ShapeID")
        .value("Unknown", adios2::ShapeID::Unknown)
        .value("GlobalValue", adios2::ShapeID::GlobalValue)
        .value("GlobalArray", adios2::ShapeID::GlobalArray)
        .value("LocalValue", adios2::ShapeID::LocalValue)
        .value("LocalArray", adios2::ShapeID::LocalArray)
        .export_values();

    pybind11::enum_<adios2::StepMode>(m, "StepMode")
        .value("Append", adios2::StepMode::Append)
        .value("Update", adios2::StepMode::Update)
        .value("Read", adios2::StepMode::Read)
        .export_values();

    pybind11::enum_<adios2::StepStatus>(m, "StepStatus")
        .value("OK", adios2::StepStatus::OK)
        .value("NotReady", adios2::StepStatus::NotReady)
        .value("EndOfStream", adios2::StepStatus::EndOfStream)
        .value("OtherError", adios2::StepStatus::OtherError)
        .export_values();

#if ADIOS2_USE_MPI
    m.def("open", &OpenMPI, pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("comm"), pybind11::arg("engine_type") = "BPFile", R"md(
          Simple API MPI open, based on python IO. 
          Allows for passing parameters in source code.

          Parameters
              name 
                    stream name
              mode
                    "w" : write, 
                    "r" : read, 
                    "a" : append (append not yet supported)
              
              comm  (mpi4py)
                    MPI communicator
              
              engine_type
                  adios2 engine type, default=BPFile

          Returns 
              file (adios2 stream)
                  handler to adios File for the simple Python API
    )md");

    m.def("open", &OpenConfigMPI, pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("comm"), pybind11::arg("config_file"),
          pybind11::arg("io_in_config_file"), R"md(
          Simple API MPI open, based on python IO. 
          Allows for passing a runtime configuration file in xml format and the
          name of the io element related to the returning File.

          Parameters
              name
                    stream name
              mode
                    "w" : write, 
                    "r" : read, 
                    "a" : append (append not yet supported)
              
              comm  (mpi4py)
                    MPI communicator
              
              config_file
                    adios2 runtime configuration file name, in xml format

              io_in_config_file
                    io element in configfile related to returning File

          Returns 
              file (adios2 stream)
                  handler to adios File for the simple Python API
    )md");

#endif
    m.def("open", &Open, "High-level API, file object open",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("engine_type") = "BPFile");

    m.def("open", &OpenConfig,
          "High-level API, file object open with a runtime config file",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("config_file"), pybind11::arg("io_in_config_file"));

    pybind11::class_<adios2::py11::ADIOS>(m, "ADIOS")
        // Python 2
        .def("__nonzero__",
             [](const adios2::py11::ADIOS &adios) {
                 const bool opBool = adios ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::ADIOS &adios) {
                 const bool opBool = adios ? true : false;
                 return opBool;
             })
        .def(pybind11::init(), "adios2 module starting point "
                               "non-MPI, constructs an ADIOS class "
                               "object")
        .def(pybind11::init<const std::string &>(),
             "adios2 module starting point non-MPI, constructs an ADIOS class "
             "object",
             pybind11::arg("configFile"))
#if ADIOS2_USE_MPI
        .def(pybind11::init<const adios2::py11::MPI4PY_Comm>(),
             "adios2 module starting point, constructs an ADIOS class object",
             pybind11::arg("comm"))
        .def(pybind11::init<const std::string &,
                            const adios2::py11::MPI4PY_Comm>(),
             "adios2 module starting point, constructs an ADIOS class object",
             pybind11::arg("configFile"), pybind11::arg("comm"))
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

    pybind11::class_<adios2::py11::IO>(m, "IO")
        // Python 2
        .def("__nonzero__",
             [](const adios2::py11::IO &io) {
                 const bool opBool = io ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::IO &io) {
                 const bool opBool = io ? true : false;
                 return opBool;
             })
        .def("SetEngine", &adios2::py11::IO::SetEngine)
        .def("SetParameters", &adios2::py11::IO::SetParameters,
             pybind11::arg("parameters") = adios2::Params())
        .def("SetParameter", &adios2::py11::IO::SetParameter)
        .def("Parameters", &adios2::py11::IO::Parameters)
        .def("AddTransport", &adios2::py11::IO::AddTransport,
             pybind11::arg("type"),
             pybind11::arg("parameters") = adios2::Params())

        .def("DefineVariable",
             (adios2::py11::Variable(adios2::py11::IO::*)(
                 const std::string &, const pybind11::array &,
                 const adios2::Dims &, const adios2::Dims &,
                 const adios2::Dims &, const bool)) &
                 adios2::py11::IO::DefineVariable,
             pybind11::return_value_policy::move, pybind11::arg("name"),
             pybind11::arg("array"), pybind11::arg("shape") = adios2::Dims(),
             pybind11::arg("start") = adios2::Dims(),
             pybind11::arg("count") = adios2::Dims(),
             pybind11::arg("isConstantDims") = false)

        .def(
            "DefineVariable",
            (adios2::py11::Variable(adios2::py11::IO::*)(const std::string &)) &
                adios2::py11::IO::DefineVariable,
            pybind11::return_value_policy::move, pybind11::arg("name"))

        .def("InquireVariable", &adios2::py11::IO::InquireVariable,
             pybind11::return_value_policy::move)

        .def("InquireAttribute", &adios2::py11::IO::InquireAttribute,
             pybind11::return_value_policy::move)

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const pybind11::array &,
                 const std::string &, const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::return_value_policy::move)

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const std::string &, const std::string &,
                 const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::arg("name"), pybind11::arg("stringValue"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::return_value_policy::move)

        .def("DefineAttribute",
             (adios2::py11::Attribute(adios2::py11::IO::*)(
                 const std::string &, const std::vector<std::string> &,
                 const std::string &, const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::arg("name"), pybind11::arg("strings"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::return_value_policy::move)

        .def("Open", (adios2::py11::Engine(adios2::py11::IO::*)(
                         const std::string &, const int)) &
                         adios2::py11::IO::Open)
#if ADIOS2_USE_MPI
        .def("Open", (adios2::py11::Engine(adios2::py11::IO::*)(
                         const std::string &, const int,
                         adios2::py11::MPI4PY_Comm comm)) &
                         adios2::py11::IO::Open)
#endif
        .def("AvailableVariables", &adios2::py11::IO::AvailableVariables)
        .def("AvailableAttributes", &adios2::py11::IO::AvailableAttributes)
        .def("FlushAll", &adios2::py11::IO::FlushAll)
        .def("EngineType", &adios2::py11::IO::EngineType)
        .def("RemoveVariable", &adios2::py11::IO::RemoveVariable)
        .def("RemoveAllVariables", &adios2::py11::IO::RemoveAllVariables)
        .def("RemoveAttribute", &adios2::py11::IO::RemoveAttribute)
        .def("RemoveAllAttributes", &adios2::py11::IO::RemoveAllAttributes);

    pybind11::class_<adios2::py11::Query>(m, "Query")
        .def("__nonzero__",
             [](const adios2::py11::Query &query) {
                 const bool opBool = query ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::Query &query) {
                 const bool opBool = query ? true : false;
                 return opBool;
             })
        .def(
            pybind11::init<const std::string &, const adios2::py11::Engine &>(),
            "adios2 query construction, a xml query File and a read engine",
            pybind11::arg("queryFile"), pybind11::arg("reader") = true)

        .def("GetResult", &adios2::py11::Query::GetResult);

    pybind11::class_<adios2::py11::Variable>(m, "Variable")
        // Python 2
        .def("__nonzero__",
             [](const adios2::py11::Variable &variable) {
                 const bool opBool = variable ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::Variable &variable) {
                 const bool opBool = variable ? true : false;
                 return opBool;
             })
        .def("SetShape", &adios2::py11::Variable::SetShape)
        .def("SetBlockSelection", &adios2::py11::Variable::SetBlockSelection)
        .def("SetSelection", &adios2::py11::Variable::SetSelection)
        .def("SetStepSelection", &adios2::py11::Variable::SetStepSelection)
        .def("SelectionSize", &adios2::py11::Variable::SelectionSize)
        .def("Name", &adios2::py11::Variable::Name)
        .def("Type", &adios2::py11::Variable::Type)
        .def("Sizeof", &adios2::py11::Variable::Sizeof)
        .def("ShapeID", &adios2::py11::Variable::ShapeID)
        .def("Shape", &adios2::py11::Variable::Shape,
             pybind11::arg("step") = adios2::EngineCurrentStep)
        .def("Start", &adios2::py11::Variable::Start)
        .def("Count", &adios2::py11::Variable::Count)
        .def("Steps", &adios2::py11::Variable::Steps)
        .def("StepsStart", &adios2::py11::Variable::StepsStart)
        .def("BlockID", &adios2::py11::Variable::BlockID)
        .def("AddOperation", &adios2::py11::Variable::AddOperation)
        .def("Operations", &adios2::py11::Variable::Operations);

    pybind11::class_<adios2::py11::Attribute>(m, "Attribute")
        // Python 2
        .def("__nonzero__",
             [](const adios2::py11::Attribute &attribute) {
                 const bool opBool = attribute ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::Attribute &attribute) {
                 const bool opBool = attribute ? true : false;
                 return opBool;
             })
        .def("Name", &adios2::py11::Attribute::Name)
        .def("Type", &adios2::py11::Attribute::Type)
        .def("DataString", &adios2::py11::Attribute::DataString)
        .def("Data", &adios2::py11::Attribute::Data);

    pybind11::class_<adios2::py11::Engine>(m, "Engine")
        // Python 2
        .def("__nonzero__",
             [](const adios2::py11::Engine &engine) {
                 const bool opBool = engine ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::Engine &engine) {
                 const bool opBool = engine ? true : false;
                 return opBool;
             })
        .def("BeginStep",
             (adios2::StepStatus(adios2::py11::Engine::*)(
                 const adios2::StepMode, const float)) &
                 adios2::py11::Engine::BeginStep,
             pybind11::arg("mode"), pybind11::arg("timeoutSeconds") = -1.f,
             pybind11::return_value_policy::move)

        .def("BeginStep",
             (adios2::StepStatus(adios2::py11::Engine::*)()) &
                 adios2::py11::Engine::BeginStep,
             pybind11::return_value_policy::move)

        .def("Put",
             (void (adios2::py11::Engine::*)(adios2::py11::Variable,
                                             const pybind11::array &,
                                             const adios2::Mode launch)) &
                 adios2::py11::Engine::Put,
             pybind11::arg("variable"), pybind11::arg("array"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("Put", (void (adios2::py11::Engine::*)(adios2::py11::Variable,
                                                    const std::string &)) &
                        adios2::py11::Engine::Put)

        .def("PerformPuts", &adios2::py11::Engine::PerformPuts)

        .def("PerformDataWrite", &adios2::py11::Engine::PerformDataWrite)

        .def("Get",
             (void (adios2::py11::Engine::*)(adios2::py11::Variable,
                                             pybind11::array &,
                                             const adios2::Mode launch)) &
                 adios2::py11::Engine::Get,
             pybind11::arg("variable"), pybind11::arg("array"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("Get",
             (std::string(adios2::py11::Engine::*)(adios2::py11::Variable,
                                                   const adios2::Mode launch)) &
                 adios2::py11::Engine::Get,
             pybind11::arg("variable"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("PerformGets", &adios2::py11::Engine::PerformGets)

        .def("EndStep", &adios2::py11::Engine::EndStep)

        .def("Flush", &adios2::py11::Engine::Flush)

        .def("Close", &adios2::py11::Engine::Close,
             pybind11::arg("transportIndex") = -1)

        .def("CurrentStep", &adios2::py11::Engine::CurrentStep)

        .def("Name", &adios2::py11::Engine::Name)

        .def("Type", &adios2::py11::Engine::Type)

        .def("Steps", &adios2::py11::Engine::Steps)

        .def("LockWriterDefinitions",
             &adios2::py11::Engine::LockWriterDefinitions)

        .def("LockReaderSelections",
             &adios2::py11::Engine::LockReaderSelections)

        .def("BlocksInfo", &adios2::py11::Engine::BlocksInfo);

    pybind11::class_<adios2::py11::Operator>(m, "Operator")
        // Python 2
        .def("__nonzero__",
             [](const adios2::py11::Operator &op) {
                 const bool opBool = op ? true : false;
                 return opBool;
             })
        // Python 3
        .def("__bool__",
             [](const adios2::py11::Operator &op) {
                 const bool opBool = op ? true : false;
                 return opBool;
             })
        .def("Type", &adios2::py11::Operator::Type)
        .def("SetParameter", &adios2::py11::Operator::SetParameter)
        .def("Parameters", &adios2::py11::Operator::Parameters);

    pybind11::class_<adios2::py11::File>(m, "File")
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

        .def("set_parameter", &adios2::py11::File::SetParameter,
             pybind11::arg("key"), pybind11::arg("value"), R"md(
             Sets a single parameter. Overwrites value if key exists.

             Parameters
                 key 
                     input parameter key

                 value
                     parameter value
        )md")

        .def("set_parameters", &adios2::py11::File::SetParameters,
             pybind11::arg("parameters"), R"md(
             Sets parameters using a dictionary. 
             Removes any previous parameter.

             Parameters
                 parameters 
                     input key/value parameters

                 value
                     parameter value
        )md")

        .def("add_transport", &adios2::py11::File::AddTransport,
             pybind11::return_value_policy::move, pybind11::arg("type"),
             pybind11::arg("parameters") = adios2::Params(), R"md(
             Adds a transport and its parameters to current IO. Must be
             supported by current engine type.

             Parameters
                 type
                     must be a supported transport type for current engine. 
                     
                 parameters
                     acceptable parameters for a particular transport
                     CAN'T use the keywords "Transport" or "transport" in key

             Returns
                 transport_index 
                     handler to added transport
        )md")

        .def("available_variables", &adios2::py11::File::AvailableVariables,
             pybind11::return_value_policy::move,
             pybind11::arg("keys") = std::vector<std::string>(), R"md(
             Returns a 2-level dictionary with variable information. 
             Read mode only.
             
             Parameters
                 keys
                    list of variable information keys to be extracted (case insensitive)
                    keys=['AvailableStepsCount','Type','Max','Min','SingleValue','Shape']
                    keys=['Name'] returns only the variable names as 1st-level keys
                    leave empty to return all possible keys

             Returns
                 variables dictionary
                     key 
                         variable name
                     value 
                         variable information dictionary
        )md")

        .def("available_attributes", &adios2::py11::File::AvailableAttributes,
             pybind11::return_value_policy::move, R"md(
             Returns a 2-level dictionary with attribute information. 
             Read mode only.
             
             Returns
                 attributes dictionary
                     key
                         attribute name
                     value 
                         attribute information dictionary
        )md")

        .def("write",
             (void (adios2::py11::File::*)(
                 const std::string &, const pybind11::array &,
                 const adios2::Dims &, const adios2::Dims &,
                 const adios2::Dims &, const bool)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("shape") = adios2::Dims(),
             pybind11::arg("start") = adios2::Dims(),
             pybind11::arg("count") = adios2::Dims(),
             pybind11::arg("end_step") = false,
             R"md(
             writes a self-describing array (numpy) variable

             Parameters
                 name
                     variable name

                 array
                     variable data values

                 shape
                     variable global MPI dimensions. 
                     Pass empty numpy array for local variables.

                 start
                     variable offset for current MPI rank. 
                     Pass empty numpy array for local variables.

                 count
                     variable dimension for current MPI rank. 
                     Pass a numpy array for local variables.

                 end_step 
                     end current step, begin next step and flush (default = false).
        )md")

        .def("write",
             (void (adios2::py11::File::*)(
                 const std::string &, const pybind11::array &,
                 const adios2::Dims &, const adios2::Dims &,
                 const adios2::Dims &, const adios2::vParams &, const bool)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("shape"), pybind11::arg("start"),
             pybind11::arg("count"), pybind11::arg("operations"),
             pybind11::arg("end_step") = false,
             R"md(
             writes a self-describing array (numpy) variable with operations
             e.g. compression: 'zfp', 'mgard', 'sz'

             Parameters
                 name 
                     variable name

                 array 
                     variable data values

                 shape 
                     variable global MPI dimensions. 
                     Pass empty numpy array for local variables.

                 start 
                     variable offset for current MPI rank. 
                     Pass empty numpy array for local variables.

                 count 
                     variable dimension for current MPI rank. 
                     Pass a numpy array for local variables.

                 end_step 
                     end current step, begin next step and flush (default = false).
        )md")

        .def("write",
             (void (adios2::py11::File::*)(const std::string &,
                                           const pybind11::array &, const bool,
                                           const bool)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("local_value") = false,
             pybind11::arg("end_step") = false, R"md(
                writes a self-describing single value array (numpy) variable

                 Parameters
                     name
                         variable name

                     array
                         variable data single value
                     
                     local_value
                         true: local value, false: global value
                            
                     end_step
                         end current step, begin next step and flush 
                         (default = false).
        )md")

        .def("write",
             (void (adios2::py11::File::*)(const std::string &,
                                           const std::string &, const bool,
                                           const bool)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("string"),
             pybind11::arg("local_value") = false,
             pybind11::arg("end_step") = false, R"md(
             writes a self-describing single value string variable

             Parameters
                 name
                     variable name

                 string
                     variable data single value

                 local_value
                     true: local value, false: global value

                 end_step
                     end current step, begin next step and flush 
                     (default = false).
        )md")

        .def("write_attribute",
             (void (adios2::py11::File::*)(
                 const std::string &, const pybind11::array &,
                 const std::string &, const std::string, const bool)) &
                 adios2::py11::File::WriteAttribute,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::arg("end_step") = false, R"md(
             writes a self-describing single value array (numpy) variable

             Parameters
                 name
                     attribute name

                 array
                     attribute numpy array data

                 variable_name
                     if attribute is associated with a variable

                 separator
                     concatenation string between variable_name and attribute
                     e.g. variable_name + separator + name ("var/attr")
                     Not used if variable_name is empty

                 end_step
                     end current step, begin next step and flush
                     (default = false).
        )md")

        .def("write_attribute",
             (void (adios2::py11::File::*)(
                 const std::string &, const std::string &, const std::string &,
                 const std::string, const bool)) &
                 adios2::py11::File::WriteAttribute,
             pybind11::arg("name"), pybind11::arg("string_value"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::arg("end_step") = false, R"md(
             writes a self-describing single value array (numpy) variable

             Parameters
                 name
                     attribute name

                 string_value
                     attribute single string

                 variable_name
                     if attribute is associated with a variable

                 separator
                     concatenation string between variable_name and attribute
                     e.g. variable_name + separator + name ("var/attr")
                     Not used if variable_name is empty

                 end_step 
                     end current step, begin next step and flush
                     (default = false).
        )md")

        .def("write_attribute",
             (void (adios2::py11::File::*)(
                 const std::string &, const std::vector<std::string> &,
                 const std::string &, const std::string, const bool)) &
                 adios2::py11::File::WriteAttribute,
             pybind11::arg("name"), pybind11::arg("string_array"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::arg("end_step") = false, R"md(
             writes a self-describing single value array (numpy) variable

             Parameters
                 name
                     attribute name

                 string_array
                     attribute string array

                 variable_name
                     if attribute is associated with a variable

                 separator
                     concatenation string between variable_name and attribute
                     e.g. variable_name + separator + name ("var/attr")
                     Not used if variable_name is empty

                 end_step
                     end current step, begin next step and flush
                     (default = false).
        )md")

        .def("read_string",
             (std::vector<std::string>(adios2::py11::File::*)(
                 const std::string &, const size_t)) &
                 adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("block_id") = 0,
             R"md(
             Reads string value for current step
             (use for streaming mode step by step)

             Parameters
                 name
                     string variable name

                 block_id
                     required for local variables

             Returns

                 list
                     data string values. 
                     For global values: returns 1 element 
                     For local values: returns n-block elements
                     
        )md")

        .def("read_string",
             (std::vector<std::string>(adios2::py11::File::*)(
                 const std::string &, const size_t, const size_t,
                 const size_t)) &
                 adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("step_start"),
             pybind11::arg("step_count"), pybind11::arg("block_id") = 0,
             R"md(
             Reads string value for a certain step 
             (random access mode)

             Parameters
                 name
                     string variable name

                 step_start 
                     variable step start

                 step_count 
                     variable number of steps to read from step_start

                 block_id
                     required for local variables

             Returns
                 string
                     data string values for a certain step range.
        )md")

        .def("read",
             (pybind11::array(adios2::py11::File::*)(const std::string &,
                                                     const size_t)) &
                 adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("block_id") = 0,
             R"md(
             Reads entire variable for current step 
             (streaming mode step by step)

             Parameters
                 name
                     variable name

                 block_id
                     required for local array variables

             Returns
                 array
                     values of variable name for current step.
                     Single values will have a shape={1} numpy array
        )md")

        .def("read",
             (pybind11::array(adios2::py11::File::*)(
                 const std::string &, const adios2::Dims &,
                 const adios2::Dims &, const size_t)) &
                 adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("start") = adios2::Dims(),
             pybind11::arg("count") = adios2::Dims(),
             pybind11::arg("block_id") = 0,
             R"md(
             Reads a selection piece in dimension for current step 
             (streaming mode step by step)

             Parameters
                 name
                     variable name

                 start
                     variable local offset selection (defaults to (0, 0, ...)

                 count
                     variable local dimension selection from start
                     defaults to whole array for GlobalArrays, or selected Block size
                     for LocalArrays
                 
                 block_id
                     required for local array variables

             Returns
                 array
                     values of variable name for current step
                     empty if exception is thrown
        )md")

        .def(
            "read",
            (pybind11::array(adios2::py11::File::*)(
                const std::string &, const adios2::Dims &, const adios2::Dims &,
                const size_t, const size_t, const size_t)) &
                adios2::py11::File::Read,
            pybind11::return_value_policy::take_ownership,
            pybind11::arg("name"), pybind11::arg("start"),
            pybind11::arg("count"), pybind11::arg("step_start"),
            pybind11::arg("step_count"), pybind11::arg("block_id") = 0, R"md(
            Random access read allowed to select steps, 
            only valid with File Engines

            Parameters
                name
                    variable to be read

                start
                    variable offset dimensions

                count
                    variable local dimensions from offset

                step_start
                    variable step start

                step_count
                    variable number of steps to read from step_start

                block_id
                    required for local array variables

            Returns
                array
                    resulting array from selection
        )md")

        .def("read_attribute",
             (pybind11::array(adios2::py11::File::*)(
                 const std::string &, const std::string &, const std::string)) &
                 adios2::py11::File::ReadAttribute,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/", R"md(
             Reads a numpy based attribute

             Parameters
                 name
                     attribute name

                 variable_name
                     if attribute is associated with a variable

                 separator
                     concatenation string between variable_name and attribute
                     e.g. variable_name + separator + name (var/attr)
                     Not used if variable_name is empty

             Returns
                 array
                     resulting array attribute data
        )md")

        .def("read_attribute_string",
             (std::vector<std::string>(adios2::py11::File::*)(
                 const std::string &, const std::string &, const std::string)) &
                 adios2::py11::File::ReadAttributeString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/", R"md(
             Read a string attribute

             Parameters
                 name
                     attribute name
                 
                 variable_name
                     if attribute is associated with a variable

                 separator
                     concatenation string between variable_name and attribute
                     e.g. variable_name + separator + name (var/attr)
                     Not used if variable_name is empty

            Returns
                 list 
                     resulting string list attribute data)md")

        .def("end_step", &adios2::py11::File::EndStep, R"md(
            Write mode: advances to the next step. Convenient when declaring
            variable attributes as advancing to the next step is not attached 
            to any variable.

            Read mode: in streaming mode releases the current step (no effect 
            in file based engines) 
        )md")

        .def("close", &adios2::py11::File::Close, R"md(
            Closes file, thus becoming unreachable. 
            Not required if using open in a with-as statement.  
            Required in all other cases per-open to avoid resource leaks.
        )md")

        .def("current_step", &adios2::py11::File::CurrentStep, R"md(
            Inspect current step when using for-in loops, read mode only

            Returns
                current step
        )md")

        .def("steps", &adios2::py11::File::Steps, R"md(
            Inspect available number of steps, for file engines, read mode only

            Returns
                steps
        )md");
}
