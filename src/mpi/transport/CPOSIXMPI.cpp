/*
 * CPOSIXMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */




#include "mpi/transport/CPOSIXMPI.h"


namespace adios
{


CPOSIXMPI::CPOSIXMPI( const std::string method, const unsigned int priority, const unsigned int iteration,
                      MPI_Comm mpiComm ):
    CTransportMPI( method, priority, iteration, mpiComm )
{ }


CPOSIXMPI::~CPOSIXMPI( )
{ }


void CPOSIXMPI::Write( CVariable& variable )
{
    int rank;
    MPI_Comm_rank( m_MPIComm, &rank );

    int size;
    MPI_Comm_size( m_MPIComm, &size );

    std::cout << "Just saying Hello from CPOSIXMPI Write from process "
              << rank << "/" << size  << "\n";
}


} //end namespace
