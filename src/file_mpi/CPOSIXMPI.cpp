/*
 * CPOSIXMPI.cpp
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */

#include "file_mpi/CPOSIXMPI.h"


namespace adios
{

CPOSIXMPI::CPOSIXMPI( const std::string fileName, const SMetadata& metadata, const MPI_Comm& mpiComm ):
    CFileMPI( fileName, "POSIX_MPI", metadata, mpiComm )
{ }


CPOSIXMPI::~CPOSIXMPI( )
{ }


void CPOSIXMPI::Open( const std::string fileName, const std::string groupName, const std::string accessMode )
{
    //here just say hello from MPI processes

    int size;
    MPI_Comm_size( m_MPIComm, &size );

    int rank;
    MPI_Comm_rank( m_MPIComm, &rank );

    std::cout << " Hello ADIOS World from POSIX processor " << rank << "/" << size << "\n";
}


unsigned long int CPOSIXMPI::GroupSize( const std::string fileName, const std::string groupName ) const
{

    return 0; //for now
}


void CPOSIXMPI::Write( const std::string fileName, const std::string variableName )
{
    //empty for now
}


void CPOSIXMPI::Close( )
{
    //empty for now
}



} //end namespace
