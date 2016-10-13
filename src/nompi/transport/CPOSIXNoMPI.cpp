/*
 * CPOSIXNoMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */


#include <iostream>

#include "nompi/transport/CPOSIXNoMPI.h"


namespace adios
{


CPOSIXNoMPI::CPOSIXNoMPI( const std::string method, const unsigned int priority, const unsigned int iteration ):
    CTransportNoMPI( method, priority, iteration )
{ }


CPOSIXNoMPI::~CPOSIXNoMPI( )
{ }


void CPOSIXNoMPI::Write( const CVariable& variable )
{
    std::cout << "Just saying Hello from CPOSIXNoMPI Write\n";
}


} //end namespace

