
#include <mpi.h>

#include <stdexcept>
#include <iostream>

#include "heatTransferStructs.h"


/**
 * heatTransfer.cpp
 *
 * Recreates heat_transfer.f90 (Fortran) in C++
 *
 * Created on: Sep 29, 2016
 *     Author: William F. Godoy
 *
 * @param argc
 * @param argv
 * @return
 */
int main( int argc, char* argv [] )
{
    try
    {
        MPI_Init( &argc, &argv );

    	int rank;
    	MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    	int size;
    	MPI_Comm_size( MPI_COMM_WORLD, &size );

    	MPI_Barrier( MPI_COMM_WORLD );

    	//Have to split and create a 'world' communicator for heatTransfer only
    	const unsigned int color = 1;
    	MPI_Comm mpiHeatTransferComm;
    	MPI_Comm_split( MPI_COMM_WORLD, color, rank, &mpiHeatTransferComm );
    	MPI_Comm_rank( mpiHeatTransferComm, &rank );
    	MPI_Comm_size( mpiHeatTransferComm, &size );

    	double timeStart = MPI_Wtime( );

    	//here must implement the call to adios init function

    }
    catch( std::ios_base::failure& e ) //I/O failure (e.g. file not found)
    {
    	std::cout << "I/O base exception caught\n";
		std::cout << e.what() << std::endl;
    }
	catch( std::exception& e ) //All other exceptions
	{
		std::cout << "Exception caught\n";
		std::cout << e.what() << std::endl;
	}


	return 0;
}


