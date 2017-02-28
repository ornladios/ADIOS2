/*
 * main.cpp
 *
 * Recreates heat_transfer.f90 (Fortran) ADIOS tutorial example in C++
 *
 * Created on: Feb 2017
 *     Author: Norbert Podhorszki
 *
 */
#include <mpi.h>

#include <stdexcept>
#include <memory>
#include <iostream>
#include <string>


#include "Settings.h"
#include "IO.h"
#include "HeatTransfer.h"

void printUsage()
{
    std::cout << "Usage: heatTransfer  output  N  M   nx  ny   steps iterations\n"
              << "  output: name of output file\n"
              << "  N:      number of processes in X dimension\n"
              << "  M:      number of processes in Y dimension\n"
              << "  nx:     local array size in X dimension per processor\n"
              << "  ny:     local array size in Y dimension per processor\n"
              << "  steps:  the total number of steps to output\n"
              << "  iterations: one step consist of this many iterations\n\n";
}


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    /* World comm spans all applications started with the same aprun command
       on a Cray XK6. So we have to split and create the local
       'world' communicator for heat_transfer only.
       In normal start-up, the communicator will just equal the MPI_COMM_WORLD.
    */

    int wrank, wnproc;
    MPI_Comm_rank( MPI_COMM_WORLD, &wrank );
    MPI_Comm_size( MPI_COMM_WORLD, &wnproc );
    MPI_Barrier( MPI_COMM_WORLD );

    const unsigned int color = 1;
    MPI_Comm mpiHeatTransferComm;
    MPI_Comm_split( MPI_COMM_WORLD, color, wrank, &mpiHeatTransferComm );

    int rank, nproc;
    MPI_Comm_rank( mpiHeatTransferComm, &rank );
    MPI_Comm_size( mpiHeatTransferComm, &nproc );

    try
    {
        double timeStart = MPI_Wtime();
        std::shared_ptr<Settings> settings( new Settings( argc, argv, rank, nproc ));
        std::shared_ptr<HeatTransfer> ht( new HeatTransfer( settings ));
        std::shared_ptr<IO> io( new IO( settings, mpiHeatTransferComm ));

        ht->init(true);
        ht->printT("Initialized T:", mpiHeatTransferComm);
        ht->heatEdges();
        //ht->exchange( mpiHeatTransferComm );
        ht->printT("Heated T:", mpiHeatTransferComm);
        io->write( 0, ht, settings, mpiHeatTransferComm );

        for( int t = 1; t <= settings->steps; ++t )
        {
            if( rank == 0 )
                std::cout << "Step " << t << ":\n";
            for( int iter = 1; iter <= settings->iterations; ++iter )
            {
                ht->iterate();
                ht->exchange( mpiHeatTransferComm );
                ht->heatEdges();
            }
            io->write( t, ht, settings, mpiHeatTransferComm );
        }
        MPI_Barrier( mpiHeatTransferComm );

        double timeEnd = MPI_Wtime();
        if( rank == 0 )
            std::cout << "Total runtime = " << timeEnd-timeStart << "s\n";

    }
    catch( std::invalid_argument& e ) // command-line argument errors
    {
        std::cout << e.what() << std::endl;
        printUsage();
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

	MPI_Finalize();
	return 0;
}


/*std::unique_ptr<Settings> ProcessArguments( int argc, char* argv [], int rank, int nproc )
{
    auto s = std::unique_ptr<Settings>( new Settings );

    if (argc < 8)
    {
        throw std::invalid_argument( "Not enough arguments" );
    }

    s->outputfile = argv[1];
    s->npx = convertToInt("N", argv[2]);
    s->npy = convertToInt("M", argv[3]);
    s->ndx = convertToInt("nx", argv[4]);
    s->ndy = convertToInt("ny", argv[5]);
    s->steps = convertToInt("steps", argv[6]);
    s->iterations = convertToInt("iterations", argv[7]);

    if( s->npx * s->npy != nproc )
    {
        throw std::invalid_argument( "N*M must equal the number of processes" );
    }

    // calculate global array size and the local offsets in that global space
    s->gndx  = s->npx * s->ndx;
    s->gndy  = s->npy * s->ndy;
    s->posx  = rank % s->npx;
    s->posy  = rank / s->npx;
    s->offsx = s->posx * s->ndx;
    s->offsy = s->posy * s->ndy;

    // determine neighbors
    if( s->posx == 0 )
        s->rank_left = -1;
    else
        s->rank_left = rank-1;

    if( s->posx == s->npx-1 )
        s->rank_right = -1;
    else
        s->rank_right = rank+1;

    if( s->posy == 0 )
        s->rank_up = -1;
    else
        s->rank_up = rank - s->npx;

    if( s->posy == s->npy-1 )
        s->rank_down = -1;
    else
        s->rank_down = rank + s->npx;

    return s;
}*/
