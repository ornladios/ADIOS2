/*
 * helloMPI.cpp
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */



#include <stdexcept>
#include <mpi.h>
#include <iostream>

#include "ADIOS.h"


int main( int argc, char* argv [] )
{
    try
    {
        MPI_Init( &argc, &argv );

        //testing with CPOSIXMPI
        adios::ADIOS adiosFile( "writer.xml", MPI_COMM_WORLD );
        adiosFile.Init( );
        MPI_Barrier( MPI_COMM_WORLD );

    }
    catch( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        std::cout << e.what() << "\n";
    }

    MPI_Finalize( );


    return 0;
}
