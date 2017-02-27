/*
 * helloWriter.cpp
 *
 *  Created on: Feb 16, 2017
 *      Author: wfg
 */



#include <vector>
#include <iostream>


#include <mpi.h>


#include "ADIOS_CPP.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    const bool adiosDebug = true;
    adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );

    try
    {
        //Define method for engine creation, it is basically straight-forward parameters
        adios::Method& bpWriterSettings = adios.DeclareMethod( "SingleFile" ); //default method type is BPWriter
        bpWriterSettings.AddTransport( "File" ); //uses default POSIX library

        //Create engine smart pointer due to polymorphism,
        //Open returns a smart pointer to Engine containing the Derived class Writer
        auto bpWriter = adios.Open( "myDoubles.bp", "r", bpWriterSettings );

        if( bpWriter == nullptr )
            throw std::ios_base::failure( "ERROR: couldn't create bpWriter at Open\n" );


    }
    catch( std::invalid_argument& e )
    {
        if( rank == 0 )
        {
            std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch( std::ios_base::failure& e )
    {
        if( rank == 0 )
        {
            std::cout << "System exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }
    catch( std::exception& e )
    {
        if( rank == 0 )
        {
            std::cout << "Exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
    }

    MPI_Finalize( );

    return 0;

}
