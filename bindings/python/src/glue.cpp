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
#include "adiosPyFunctions.h"


namespace bpy = boost::python;
namespace np = boost::python::numpy;


adios::ADIOSPy ADIOSPy( bpy::object py_comm, const bool debug )
{
    MPI_Comm* comm_p = PyMPIComm_Get( py_comm.ptr() );
    if (comm_p == NULL) bpy::throw_error_already_set();
    return adios::ADIOSPy( *comm_p, debug );
}


using ReturnInternalReference = bpy::return_internal_reference<>;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( open_overloads, adios::ADIOSPy::OpenPy, 3, 4 )

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( write_overload, adios::EnginePy::WritePy, 2, 2 )



BOOST_PYTHON_MODULE( ADIOSPy )
{
    if (import_mpi4py() < 0) return; /* Python 2.X */

    Py_Initialize();
    np::initialize();


    bpy::class_< adios::Dims >("Dims")
        .def(boost::python::vector_indexing_suite< adios::Dims >() );
    //functions
    bpy::def("ADIOSPy", ADIOSPy );

    //classes
    bpy::class_<adios::ADIOSPy>("ADIOS", bpy::no_init )
        .def("HelloMPI", &adios::ADIOSPy::HelloMPI )
        .def("DefineVariableDouble", &adios::ADIOSPy::DefineVariablePy<double>, ReturnInternalReference() )
        .def("DefineVariableFloat", &adios::ADIOSPy::DefineVariablePy<float>, ReturnInternalReference() )
        .def("DeclareMethod", &adios::ADIOSPy::DeclareMethodPy, ReturnInternalReference() )
		.def("Open", &adios::ADIOSPy::OpenPy, open_overloads() )
    ;

    bpy::class_<adios::VariablePy<double>>("VariableDouble", bpy::no_init )
		.def("SetLocalDimensions", &adios::VariablePy<double>::SetLocalDimensions )
		.def("GetLocalDimensions", &adios::VariablePy<double>::GetLocalDimensions )
	;

    bpy::class_<adios::MethodPy>("Method", bpy::no_init )
        .def("SetParameters", bpy::raw_function( &adios::MethodPy::SetParametersPy, 1 )  )
        .def("AddTransport", bpy::raw_function( &adios::MethodPy::AddTransportPy, 1 ) )
        .def("PrintAll", &adios::MethodPy::PrintAll )
    ;

    //Engines
    bpy::class_<adios::EnginePy>("EnginePy", bpy::no_init )
    	.def( "GetType", &adios::EnginePy::GetType )
		.def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<double>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
	    .def("Write", static_cast< void( adios::EnginePy::*)
	        (adios::VariablePy<float>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
		.def( "Close", &adios::EnginePy::Close )
    ;

}
