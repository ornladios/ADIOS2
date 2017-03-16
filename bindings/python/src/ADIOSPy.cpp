/*
 * ADIOSPy.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#include <iostream>

#include <mpi4py/mpi4py.h>

#include "ADIOSPy.h"

#include "core/Engine.h"

namespace adios
{


ADIOSPy::ADIOSPy( MPI_Comm mpiComm, const bool debug ):
    ADIOS( mpiComm, debug )
{ }


ADIOSPy::~ADIOSPy( )
{ }


void ADIOSPy::HelloMPI( )
{
    std::cout << "Hello ADIOSPy from rank " << m_RankMPI << "/" << m_SizeMPI << "\n";
}


MethodPy& ADIOSPy::DeclareMethodPy( const std::string methodName, const std::string type )
{
    Method& method = DeclareMethod( methodName, type );
    return *reinterpret_cast<MethodPy*>( &method );
}


EnginePy ADIOSPy::OpenPy( const std::string name, const std::string accessMode,
    		              const MethodPy& method, boost::python::object py_comm )
{
	EnginePy enginePy;
	if( py_comm == boost::python::object() ) //None
	{
		enginePy.m_Engine = Open( name, accessMode, method );
	}
	else
	{
		if (import_mpi4py() < 0) throw std::logic_error( "ERROR: could not import mpi4py communicator in Open " + name + "\n" );
		MPI_Comm* comm_p = PyMPIComm_Get( py_comm.ptr() );
		if( comm_p == nullptr ) boost::python::throw_error_already_set();

		enginePy.m_Engine = Open( name, accessMode, *comm_p, method );
	}
	//here downcast
	return enginePy;
}




} //end namespace


