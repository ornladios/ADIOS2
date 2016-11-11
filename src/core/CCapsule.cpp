/*
 * CCapsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */

#include "core/CCapsule.h"


namespace adios
{

CCapsule::CCapsule( const MPI_Comm mpiComm, const unsigned long int bufferSize ):
    m_MPIComm{ mpiComm },
    m_BufferSize{ bufferSize }
{ }


CCapsule::~CCapsule( )
{ }





}
