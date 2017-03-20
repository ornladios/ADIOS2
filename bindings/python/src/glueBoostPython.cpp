/*
 * glue.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#include <mpi4py/mpi4py.h>

#include "boost/python.hpp"
#include "boost/python/suite/indexing/vector_indexing_suite.hpp"
#include "boost/python/raw_function.hpp"
#include "boost/python/numpy.hpp"


#include "ADIOSPy.h"
#include "EnginePy.h"
#include "adiosPyFunctions.h"


namespace py = boost::python;
namespace np = boost::python::numpy;


adios::ADIOSPy ADIOSPy( py::object py_comm, const bool debug )
{
    MPI_Comm* comm_p = PyMPIComm_Get( py_comm.ptr() );
    if (comm_p == NULL) py::throw_error_already_set();
    return adios::ADIOSPy( *comm_p, debug );
}


using ReturnInternalReference = py::return_internal_reference<>;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( open_overloads, adios::ADIOSPy::OpenPy, 3, 4 )


BOOST_PYTHON_MODULE( ADIOSPy )
{
    if (import_mpi4py() < 0) return; /* Python 2.X */

    Py_Initialize();
    np::initialize();

    py::class_< adios::Dims >("Dims")
        .def(boost::python::vector_indexing_suite< adios::Dims >() );
    //functions
    py::def("ADIOSPy", ADIOSPy );

    //classes
    py::class_<adios::ADIOSPy>("ADIOS", py::no_init )
        .def("HelloMPI", &adios::ADIOSPy::HelloMPI )
        .def("DefineVariable", &adios::ADIOSPy::DefineVariablePy )
        .def("DeclareMethod", &adios::ADIOSPy::DeclareMethodPy, ReturnInternalReference() )
		.def("Open", &adios::ADIOSPy::OpenPy, open_overloads() )
    ;

    py::class_<adios::VariablePy>("Variable", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy::GetLocalDimensions )
    ;

    py::class_<adios::MethodPy>("Method", py::no_init )
        .def("SetParameters", py::raw_function( &adios::MethodPy::SetParametersPy, 1 )  )
        .def("AddTransport", py::raw_function( &adios::MethodPy::AddTransportPy, 1 ) )
        .def("PrintAll", &adios::MethodPy::PrintAll )
    ;

    //Engine
    py::class_<adios::EnginePy>("EnginePy", py::no_init )
        .def("Write", &adios::EnginePy::WritePy )
		.def( "Close", &adios::EnginePy::Close )
    ;
}

