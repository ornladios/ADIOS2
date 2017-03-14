/*
 * glue.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#include <mpi4py/mpi4py.h>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include "ADIOSPy.h"
#include "adiosPyFunctions.h"


adios::ADIOSPy ADIOSPy( boost::python::object py_comm, const bool debug )
{
    MPI_Comm* comm_p = PyMPIComm_Get( py_comm.ptr() );
    if (comm_p == NULL) boost::python::throw_error_already_set();
    return adios::ADIOSPy( *comm_p, debug );
}


BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( d_overloads, DefineVariableDouble, 1, 4 )
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( f_overloads, DefineVariableFloat, 1, 4 )


BOOST_PYTHON_MODULE( ADIOSPy )
{
    if (import_mpi4py() < 0) return; /* Python 2.X */


    boost::python::class_<std::vector<std::size_t> >("Dims")
        .def(boost::python::vector_indexing_suite< std::vector<std::size_t> >() );
    //functions
    boost::python::def("ADIOSPy", ADIOSPy );

    //classes
    boost::python::class_<adios::ADIOSPy>("ADIOS", boost::python::no_init )
        .def("HelloMPI", &adios::ADIOSPy::HelloMPI )
        .def("DefineVariableDouble", &adios::ADIOSPy::DefineVariableDouble,
            boost::python::return_value_policy<boost::python::reference_existing_object>(), d_overloads() )
        .def("DefineVariableFloat", &adios::ADIOSPy::DefineVariableFloat, f_overloads() )
        .def("SetVariableLocalDimensions", &adios::ADIOSPy::SetVariableLocalDimensions )
        .def("GetVariableLocalDimensions", &adios::ADIOSPy::GetVariableLocalDimensions )
    ;

    //classes
    boost::python::class_<adios::VariablePy<double>>("VariableDouble", boost::python::no_init )
		.def("SetLocalDimensions", &adios::VariablePy<double>::SetLocalDimensions )
		.def("GetLocalDimensions", &adios::VariablePy<double>::GetLocalDimensions )
	;


}
