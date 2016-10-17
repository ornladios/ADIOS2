/*
 * CPOSIXMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#include <iostream>

#include "include/mpi/transport/CPOSIX.h"


namespace adios
{


CPOSIX::CPOSIX( const unsigned int priority, const unsigned int iteration,
                MPI_Comm mpiComm ):
    CTransportMPI( "POSIX", priority, iteration, mpiComm )
{ }


CPOSIX::~CPOSIX( )
{ }


void CPOSIX::Write( const CVariable& variable )
{
    int rank;
    MPI_Comm_rank( m_MPIComm, &rank );

    int size;
    MPI_Comm_size( m_MPIComm, &size );

    std::cout << "Just saying Hello from CPOSIXMPI Write from process "
              << rank << "/" << size  << "\n";
}


} //end namespace
