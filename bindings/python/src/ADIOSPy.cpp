/*
 * ADIOSPy.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: wfg
 */

#include <iostream>

#include "ADIOSPy.h"

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





} //end namespace


