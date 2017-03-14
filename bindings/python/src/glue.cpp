/*
 * glue.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#include <mpi4py/mpi4py.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/raw_function.hpp>

#include "ADIOSPy.h"
#include "adiosPyFunctions.h"


adios::ADIOSPy ADIOSPy( boost::python::object py_comm, const bool debug )
{
    MPI_Comm* comm_p = PyMPIComm_Get( py_comm.ptr() );
    if (comm_p == NULL) boost::python::throw_error_already_set();
    return adios::ADIOSPy( *comm_p, debug );
}



using ReturnInternalReference = boost::python::return_internal_reference<>;



BOOST_PYTHON_MODULE( ADIOSPy )
{
    if (import_mpi4py() < 0) return; /* Python 2.X */


    boost::python::class_< adios::Dims >("Dims")
        .def(boost::python::vector_indexing_suite< adios::Dims >() );
    //functions
    boost::python::def("ADIOSPy", ADIOSPy );

    //classes
    boost::python::class_<adios::ADIOSPy>("ADIOS", boost::python::no_init )
        .def("HelloMPI", &adios::ADIOSPy::HelloMPI )
        .def("DefineVariableDouble", &adios::ADIOSPy::DefineVariablePy<double>, ReturnInternalReference() )
        .def("DefineVariableFloat", &adios::ADIOSPy::DefineVariablePy<float>, ReturnInternalReference() )
        .def("DeclareMethod", &adios::ADIOSPy::DeclareMethodPy, ReturnInternalReference() )
    ;

    //classes
    boost::python::class_<adios::VariablePy<double>>("VariableDouble", boost::python::no_init )
		.def("SetLocalDimensions", &adios::VariablePy<double>::SetLocalDimensions )
		.def("GetLocalDimensions", &adios::VariablePy<double>::GetLocalDimensions )
	;

    boost::python::class_<adios::MethodPy>("Method", boost::python::no_init )
        .def("SetParameters", boost::python::raw_function( &adios::MethodPy::SetParametersPy, 1 )  )
        .def("AddTransport", boost::python::raw_function( &adios::MethodPy::AddTransportPy, 1 ) )
        .def("PrintAll", &adios::MethodPy::PrintAll )
    ;


}
