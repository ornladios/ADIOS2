/*
 * CPOSIXMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */


#include <iostream>

#include "transport/CPOSIX.h"


namespace adios
{


CPOSIX::CPOSIX( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm ):
    CTransport( "POSIX", priority, iteration, mpiComm )
{ }


CPOSIX::~CPOSIX( )
{ }


void CPOSIX::Open( const std::string fileName, const std::string accessMode )
{

}


void CPOSIX::Write( CVariableBase& variable )
{

}


void CPOSIX::Close( )
{

}



}//end namespace
