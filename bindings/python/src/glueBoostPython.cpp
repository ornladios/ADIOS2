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

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS( write_overload, adios::EnginePy::WritePy, 2, 2 )



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
        .def("DefineVariableChar", &adios::ADIOSPy::DefineVariablePy<char>, ReturnInternalReference() )
        .def("DefineVariableUChar", &adios::ADIOSPy::DefineVariablePy<unsigned char>, ReturnInternalReference() )
        .def("DefineVariableShort", &adios::ADIOSPy::DefineVariablePy<short>, ReturnInternalReference() )
        .def("DefineVariableUShort", &adios::ADIOSPy::DefineVariablePy<unsigned short>, ReturnInternalReference() )
        .def("DefineVariableInt", &adios::ADIOSPy::DefineVariablePy<int>, ReturnInternalReference() )
        .def("DefineVariableUInt", &adios::ADIOSPy::DefineVariablePy<unsigned int>, ReturnInternalReference() )
        .def("DefineVariableLInt", &adios::ADIOSPy::DefineVariablePy<long int>, ReturnInternalReference() )
        .def("DefineVariableULInt", &adios::ADIOSPy::DefineVariablePy<unsigned long int>, ReturnInternalReference() )
        .def("DefineVariableLLInt", &adios::ADIOSPy::DefineVariablePy<long long int>, ReturnInternalReference() )
        .def("DefineVariableULLInt", &adios::ADIOSPy::DefineVariablePy<unsigned long long int>, ReturnInternalReference() )
        .def("DefineVariableFloat", &adios::ADIOSPy::DefineVariablePy<float>, ReturnInternalReference() )
        .def("DefineVariableDouble", &adios::ADIOSPy::DefineVariablePy<double>, ReturnInternalReference() )
        .def("DefineVariableLDouble", &adios::ADIOSPy::DefineVariablePy<long double>, ReturnInternalReference() )
        .def("DefineVariableCFloat", &adios::ADIOSPy::DefineVariablePy<std::complex<float>>, ReturnInternalReference() )
        .def("DefineVariableCDouble", &adios::ADIOSPy::DefineVariablePy<std::complex<double>>, ReturnInternalReference() )
        .def("DefineVariableCLDouble", &adios::ADIOSPy::DefineVariablePy<std::complex<long double>>, ReturnInternalReference() )
        .def("DeclareMethod", &adios::ADIOSPy::DeclareMethodPy, ReturnInternalReference() )
		.def("Open", &adios::ADIOSPy::OpenPy, open_overloads() )
    ;

    py::class_<adios::VariablePy<char>>("VariableChar", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<char>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<char>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<unsigned char>>("VariableUChar", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<unsigned char>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<unsigned char>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<short>>("VariableShort", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<short>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<short>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<unsigned short>>("VariableUShort", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<unsigned short>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<unsigned short>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<int>>("VariableInt", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<int>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<int>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<unsigned int>>("VariableUInt", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<unsigned int>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<unsigned int>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<long int>>("VariableLInt", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<long int>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<long int>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<unsigned long int>>("VariableULInt", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<unsigned long int>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<unsigned long int>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<long long int>>("VariableLLInt", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<long long int>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<long long int>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<unsigned long long int>>("VariableULLInt", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<unsigned long long int>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<unsigned long long int>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<float>>("VariableFloat", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<float>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<float>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<double>>("VariableDouble", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<double>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<double>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<long double>>("VariableLDouble", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<long double>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<long double>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<std::complex<float>>>("VariableCFloat", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<std::complex<float>>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<std::complex<float>>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<std::complex<double>>>("VariableCDouble", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<std::complex<double>>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<std::complex<double>>::GetLocalDimensions )
    ;

    py::class_<adios::VariablePy<std::complex<long double>>>("VariableCLDouble", py::no_init )
        .def("SetLocalDimensions", &adios::VariablePy<std::complex<long double>>::SetLocalDimensions )
        .def("GetLocalDimensions", &adios::VariablePy<std::complex<long double>>::GetLocalDimensions )
    ;

    py::class_<adios::MethodPy>("Method", py::no_init )
        .def("SetParameters", py::raw_function( &adios::MethodPy::SetParametersPy, 1 )  )
        .def("AddTransport", py::raw_function( &adios::MethodPy::AddTransportPy, 1 ) )
        .def("PrintAll", &adios::MethodPy::PrintAll )
    ;

    //Engine
    py::class_<adios::EnginePy>("EnginePy", py::no_init )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<char>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<unsigned char>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<short>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<unsigned short>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<int>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<unsigned int>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<long int>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<unsigned long int>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<long long int>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<unsigned long long int>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<float>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<double>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<long double>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<std::complex<float>>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<std::complex<double>>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )
        .def("Write", static_cast< void( adios::EnginePy::*)
            (adios::VariablePy<std::complex<long double>>&, const np::ndarray& )>( &adios::EnginePy::WritePy ), write_overload() )

		.def( "Close", &adios::EnginePy::Close )
    ;
}

