/*
 * helloMPI.cpp
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */



#include <stdexcept>
#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>

#include "public/ADIOS.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    adios::ADIOS adios;

    try
    {
        //testing with CPOSIXMPI
        adios = adios::ADIOS( "writer.xml", MPI_COMM_WORLD );

        //Get Monitor info
        std::ofstream logStream( "info_" + std::to_string(rank) + ".log" );
        adios.MonitorGroups( logStream );

        MPI_Barrier( MPI_COMM_WORLD );
    }
    catch( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        if( rank == 0 )
        {
            std::cout << e.what() << "\n";
        }
    }


    MPI_Finalize( );


    return 0;
}
