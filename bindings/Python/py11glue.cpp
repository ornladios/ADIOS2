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

#include <stdexcept>

#include <adios2.h>

#ifdef ADIOS2_HAVE_MPI
#include <mpi4py/mpi4py.h>
#endif

#include "py11ADIOS.h"
#include "py11Attribute.h"
#include "py11Engine.h"
#include "py11File.h"
#include "py11IO.h"
#include "py11Operator.h"
#include "py11Variable.h"

#ifdef ADIOS2_HAVE_MPI
adios2::py11::ADIOS ADIOSInitConfig(const std::string &configFile,
                                    pybind11::object &comm,
                                    const bool debugMode)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(comm.ptr());

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

adios2::py11::ADIOS ADIOSInit(pybind11::object &comm, const bool debugMode)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(comm.ptr());

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
                        pybind11::object &comm, const std::string enginetype)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(comm.ptr());

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
    return adios2::py11::File(name, mode, *mpiCommPtr, enginetype);
}

adios2::py11::File OpenConfig(const std::string &name, const std::string mode,
                              pybind11::object &comm,
                              const std::string &configfile,
                              const std::string ioinconfigfile)
{
    MPI_Comm *mpiCommPtr = PyMPIComm_Get(comm.ptr());

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
    return adios2::py11::File(name, mode, *mpiCommPtr, configfile,
                              ioinconfigfile);
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

    m.def("open", &Open, pybind11::arg("name"), pybind11::arg("mode"),
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

    m.def("open", &OpenConfig, pybind11::arg("name"), pybind11::arg("mode"),
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
          pybind11::arg("engine_type") = "BPFile");

    m.def("open", &OpenConfig,
          "High-level API, file object open with a runtime config file",
          pybind11::arg("name"), pybind11::arg("mode"),
          pybind11::arg("config_file"), pybind11::arg("io_in_config_file"));
#endif

    pybind11::class_<adios2::py11::ADIOS>(m, "py11_ADIOS")
        .def("__nonzero__",
             [](const adios2::py11::ADIOS &adios) {
                 if (adios)
                 {
                     return true;
                 }
                 else
                 {
                     return false;
                 };
             })
        .def("DeclareIO", &adios2::py11::ADIOS::DeclareIO,
             "spawn IO object component returning a IO object with a unique "
             "name, throws an exception if IO with the same name is declared "
             "twice")
        .def("AtIO", &adios2::py11::ADIOS::AtIO, "returns an IO object "
                                                 "previously defined IO object "
                                                 "with DeclareIO, throws "
                                                 "an exception if not found")
        .def("DefineOperator", &adios2::py11::ADIOS::DefineOperator)
        .def("InquireOperator", &adios2::py11::ADIOS::InquireOperator)
        .def("FlushAll", &adios2::py11::ADIOS::FlushAll,
             "flushes all engines in all spawned IO objects");

    pybind11::class_<adios2::py11::IO>(m, "IO")
        .def("__nonzero__",
             [](const adios2::py11::IO &io) {
                 if (io)
                 {
                     return true;
                 }
                 else
                 {
                     return false;
                 };
             })
        .def("SetEngine", &adios2::py11::IO::SetEngine)
        .def("SetParameters", &adios2::py11::IO::SetParameters,
             pybind11::arg("parameters") = adios2::Params())
        .def("SetParameter", &adios2::py11::IO::SetParameter)
        .def("Parameters", &adios2::py11::IO::Parameters)
        .def("AddTransport", &adios2::py11::IO::AddTransport,
             pybind11::arg("type"),
             pybind11::arg("parameters") = adios2::Params())

        .def("DefineVariable", (adios2::py11::Variable (adios2::py11::IO::*)(
                                   const std::string &, const pybind11::array &,
                                   const adios2::Dims &, const adios2::Dims &,
                                   const adios2::Dims &, const bool)) &
                                   adios2::py11::IO::DefineVariable,
             pybind11::return_value_policy::move, pybind11::arg("name"),
             pybind11::arg("array"), pybind11::arg("shape") = adios2::Dims(),
             pybind11::arg("start") = adios2::Dims(),
             pybind11::arg("count") = adios2::Dims(),
             pybind11::arg("isConstantDims") = false)

        .def("DefineVariable", (adios2::py11::Variable (adios2::py11::IO::*)(
                                   const std::string &)) &
                                   adios2::py11::IO::DefineVariable,
             pybind11::return_value_policy::move, pybind11::arg("name"))

        .def("InquireVariable", &adios2::py11::IO::InquireVariable,
             pybind11::return_value_policy::move)

        .def("InquireAttribute", &adios2::py11::IO::InquireAttribute,
             pybind11::return_value_policy::move)

        .def("DefineAttribute",
             (adios2::py11::Attribute (adios2::py11::IO::*)(
                 const std::string &, const pybind11::array &,
                 const std::string &, const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("variableName") = "",
             pybind11::arg("separator") = "/",
             pybind11::return_value_policy::move)

        .def("DefineAttribute", (adios2::py11::Attribute (adios2::py11::IO::*)(
                                    const std::string &, const std::string &,
                                    const std::string &, const std::string)) &
                                    adios2::py11::IO::DefineAttribute,
             pybind11::arg("name"), pybind11::arg("stringValue"),
             pybind11::arg("variableName") = "",
             pybind11::arg("separator") = "/",
             pybind11::return_value_policy::move)

        .def("DefineAttribute",
             (adios2::py11::Attribute (adios2::py11::IO::*)(
                 const std::string &, const std::vector<std::string> &,
                 const std::string &, const std::string)) &
                 adios2::py11::IO::DefineAttribute,
             pybind11::arg("name"), pybind11::arg("strings"),
             pybind11::arg("variableName") = "",
             pybind11::arg("separator") = "/",
             pybind11::return_value_policy::move)

        .def("Open", (adios2::py11::Engine (adios2::py11::IO::*)(
                         const std::string &, const int)) &
                         adios2::py11::IO::Open)
        .def("AvailableVariables", &adios2::py11::IO::AvailableVariables)
        .def("AvailableAttributes", &adios2::py11::IO::AvailableAttributes)
        .def("FlushAll", &adios2::py11::IO::FlushAll)
        .def("EngineType", &adios2::py11::IO::EngineType)
        .def("LockDefinitions", &adios2::py11::IO::LockDefinitions);

    pybind11::class_<adios2::py11::Variable>(m, "Variable")
        .def("__nonzero__",
             [](const adios2::py11::Variable &variable) {
                 if (variable)
                 {
                     return true;
                 }
                 else
                 {
                     return false;
                 };
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
        .def("Shape", &adios2::py11::Variable::Shape)
        .def("Start", &adios2::py11::Variable::Start)
        .def("Count", &adios2::py11::Variable::Count)
        .def("Steps", &adios2::py11::Variable::Steps)
        .def("StepsStart", &adios2::py11::Variable::StepsStart)
        .def("BlockID", &adios2::py11::Variable::BlockID)
        .def("AddOperation", &adios2::py11::Variable::AddOperation)
        .def("Operations", &adios2::py11::Variable::Operations);

    pybind11::class_<adios2::py11::Attribute>(m, "Attribute")
        .def("__nonzero__",
             [](const adios2::py11::Attribute &attribute) {
                 if (attribute)
                 {
                     return true;
                 }
                 else
                 {
                     return false;
                 };
             })
        .def("Name", &adios2::py11::Attribute::Name)
        .def("Type", &adios2::py11::Attribute::Type)
        .def("DataString", &adios2::py11::Attribute::DataString)
        .def("Data", &adios2::py11::Attribute::Data);

    pybind11::class_<adios2::py11::Engine>(m, "Engine")
        .def("__nonzero__",
             [](const adios2::py11::Engine &engine) {
                 if (engine)
                 {
                     return true;
                 }
                 else
                 {
                     return false;
                 };
             })
        .def("BeginStep", (adios2::StepStatus (adios2::py11::Engine::*)(
                              const adios2::StepMode, const float)) &
                              adios2::py11::Engine::BeginStep,
             pybind11::arg("mode"), pybind11::arg("timeoutSeconds") = -1.f,
             pybind11::return_value_policy::move)

        .def("BeginStep", (adios2::StepStatus (adios2::py11::Engine::*)()) &
                              adios2::py11::Engine::BeginStep,
             pybind11::return_value_policy::move)

        .def("Put", (void (adios2::py11::Engine::*)(
                        adios2::py11::Variable, const pybind11::array &,
                        const adios2::Mode launch)) &
                        adios2::py11::Engine::Put,
             pybind11::arg("variable"), pybind11::arg("array"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("Put", (void (adios2::py11::Engine::*)(adios2::py11::Variable,
                                                    const std::string &)) &
                        adios2::py11::Engine::Put)

        .def("PerformPuts", &adios2::py11::Engine::PerformPuts)

        .def("Get", (void (adios2::py11::Engine::*)(
                        adios2::py11::Variable, pybind11::array &,
                        const adios2::Mode launch)) &
                        adios2::py11::Engine::Get,
             pybind11::arg("variable"), pybind11::arg("array"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("Get", (void (adios2::py11::Engine::*)(
                        adios2::py11::Variable, std::string &,
                        const adios2::Mode launch)) &
                        adios2::py11::Engine::Get,
             pybind11::arg("variable"), pybind11::arg("string"),
             pybind11::arg("launch") = adios2::Mode::Deferred)

        .def("PerformGets", &adios2::py11::Engine::PerformGets)

        .def("EndStep", &adios2::py11::Engine::EndStep)

        .def("Flush", &adios2::py11::Engine::Flush)

        .def("Close", &adios2::py11::Engine::Close,
             pybind11::arg("transportIndex") = -1)

        .def("CurrentStep", &adios2::py11::Engine::CurrentStep)

        .def("Name", &adios2::py11::Engine::Name)

        .def("Type", &adios2::py11::Engine::Type);

    pybind11::class_<adios2::py11::Operator>(m, "Operator")
        .def("__nonzero__",
             [](const adios2::py11::Operator &op) {
                 if (op)
                 {
                     return true;
                 }
                 else
                 {
                     return false;
                 };
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
                 parameters dictionary 
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
                     
                 parameters dictionary
                     acceptable parameters for a particular transport
                     CAN'T use the keywords "Transport" or "transport" in key

             Returns
                 transportindex 
                     handler to added transport
        )md")

        .def("available_variables", &adios2::py11::File::AvailableVariables,
             pybind11::return_value_policy::move, R"md(
             Returns a 2-level dictionary with variable information. 
             Read mode only.
             
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

        .def("write", (void (adios2::py11::File::*)(
                          const std::string &, const pybind11::array &,
                          const adios2::Dims &, const adios2::Dims &,
                          const adios2::Dims &, const bool)) &
                          adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("shape"), pybind11::arg("start"),
             pybind11::arg("count"), pybind11::arg("end_step") = false,
             R"md(
             writes a self-describing array (numpy) variable

             Parameters
                 name 
                     variable name

                 array: numpy 
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

                 endstep 
                     end current step, begin next step and flush (default = false).
        )md")

        .def("write_attribute",
             (void (adios2::py11::File::*)(
                 const std::string &, const pybind11::array &,
                 const std::string &, const std::string, const bool)) &
                 adios2::py11::File::WriteAttribute,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             pybind11::arg("end_step") = false,
             R"md(
		             writes a self-describing single value array (numpy) variable

		             Parameters
		                 name
		                     attribute name

		                 array: numpy 
		                     attribute numpy array data

						 variablename:
                             if attribute is associated with a variable

                         separator:
                             concatenation string between variablename and attribute
                             e.g. variablename + separator + name
                                  var/units. Not used if variablename is empty 
		                 endstep 
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
             pybind11::arg("end_step") = false,
             R"md(
				 writes a self-describing single value array (numpy) variable

				 Parameters
					 name
						 attribute name

					 stringvalue:  
						 attribute single string

					 variablename:
						 if attribute is associated with a variable

					 separator:
						 concatenation string between variablename and attribute
						 e.g. variablename + separator + name
							  var/units. Not used if variablename is empty 
					 endstep 
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
             pybind11::arg("end_step") = false,
             R"md(
				 writes a self-describing single value array (numpy) variable

				 Parameters
					 name
						 attribute name

					 stringarray:  
						 attribute string array

					 variablename:
						 if attribute is associated with a variable

					 separator:
						 concatenation string between variablename and attribute
						 e.g. variablename + separator + name
							  var/units. Not used if variablename is empty 
					 endstep 
						 end current step, begin next step and flush 
						 (default = false).
			)md")

        .def("write",
             (void (adios2::py11::File::*)(
                 const std::string &, const pybind11::array &, const bool)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("array"),
             pybind11::arg("end_step") = false, R"md(
             writes a self-describing single value array (numpy) variable

             Parameters
                 name
                     variable name

                 array: numpy 
                     variable data single value

                 endstep 
                     end current step, begin next step and flush 
                     (default = false).
        )md")

        .def("write",
             (void (adios2::py11::File::*)(const std::string &,
                                           const std::string &, const bool)) &
                 adios2::py11::File::Write,
             pybind11::arg("name"), pybind11::arg("string"),
             pybind11::arg("end_step") = false, R"md(
             writes a self-describing single value string variable

             Parameters
                 name
                     variable name

                 string 
                     variable data single value

                 endstep 
                     end current step, begin next step and flush 
                     (default = false).
        )md")

        .def("read_string", (std::vector<std::string> (adios2::py11::File::*)(
                                const std::string &)) &
                                adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"),
             R"md(
             Reads string value for current step 
             (use for streaming mode step by step)

             Parameters
                 name
                     string variable name

             Returns
                 list string
                     data string values. For global values, returns 1 element list, 
					 for local_values an n-block size list
        )md")

        .def("read_string",
             (std::vector<std::string> (adios2::py11::File::*)(
                 const std::string &, const size_t, const size_t)) &
                 adios2::py11::File::ReadString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("step_start"),
             pybind11::arg("step_count"),
             R"md(
             Reads string value for a certain step 
             (random access mode)

             Parameters
                 name
                     string variable name
                 step
                     input step to be read

             Returns
                 string list
                     data string values for a certain step range.
        )md")

        .def("read",
             (pybind11::array (adios2::py11::File::*)(const std::string &)) &
                 adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), R"md(
             Reads entire variable for current step 
             (streaming mode step by step)

             Parameters
                 name
                        variable name

             Returns
                 array: numpy
                        values of variable name for current step.
                        Single values will have a shape={1} numpy array
        )md")

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("start"),
             pybind11::arg("count"), R"md(
             Reads a selection piece in dimension for current step 
             (streaming mode step by step)

             Parameters
                 name
                     variable name

                 start
                     variable local offset selection

                 count
                     variable local dimension selection from start

             Returns
                 array: numpy
                     values of variable name for current step
                     empty if exception is thrown
        )md")

        .def("read", (pybind11::array (adios2::py11::File::*)(
                         const std::string &, const adios2::Dims &,
                         const adios2::Dims &, const size_t, const size_t)) &
                         adios2::py11::File::Read,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("start"),
             pybind11::arg("count"), pybind11::arg("step_start"),
             pybind11::arg("step_count"), R"md(
             Random access read allowed to select steps, 
             only valid with File Engines

             Parameters
                 name 
                     variable to be read 

                 start 
                     variable offset dimensions

                 count 
                     variable local dimensions from offset

                 stepstart 
                     variable step start

                 stepcount 
                     variable number of steps to read 

             Returns
                 array: numpy
                    resulting array from selection 
        )md")

        .def("read_attribute",
             (pybind11::array (adios2::py11::File::*)(
                 const std::string &, const std::string &, const std::string)) &
                 adios2::py11::File::ReadAttribute,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             R"md(
				 Reads a numpy based attribute

				 Parameters
					 name
						 attribute name
					 
					 variablename:
						 if attribute is associated with a variable

					 separator:
						 concatenation string between variablename and attribute
						 e.g. variablename + separator + name
							  var/units. Not used if variablename is empty 
				Returns
		 			 array: numpy
						 resulting array attribute data
	    )md")

        .def("read_attribute_string",
             (std::vector<std::string> (adios2::py11::File::*)(
                 const std::string &, const std::string &, const std::string)) &
                 adios2::py11::File::ReadAttributeString,
             pybind11::return_value_policy::take_ownership,
             pybind11::arg("name"), pybind11::arg("variable_name") = "",
             pybind11::arg("separator") = "/",
             R"md(
				 Read a string attribute

				 Parameters
					 name
						 attribute name
					 
					 variablename:
						 if attribute is associated with a variable

					 separator:
						 concatenation string between variablename and attribute
						 e.g. variablename + separator + name
							  var/units. Not used if variablename is empty 
				Returns
					 list: 
						 resulting string list attribute data 
		)md")

        .def("end_step", &adios2::py11::File::EndStep, R"md(
			Write mode: advances to the next step.
                        Convenient when declaring variable attributes 
                        as advancing to the next
			            step is not attached to any variable.
			Read mode: in streaming mode releases the current step 
                       (no effect in file based engines) 
		)md")

        .def("close", &adios2::py11::File::Close, R"md(
            Closes file, thus becoming unreachable. 
            Not required if using open in a with-as statement.  
            Required in all other cases per-open to avoid resource leaks.
        )md")

        .def("current_step", &adios2::py11::File::CurrentStep, R"md(
            Return current step when using for-in loops, read mode only

            Returns
                current step
        )md");
}
