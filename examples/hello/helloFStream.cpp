/*
 * helloFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */


#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <numeric>

#ifdef HAVE_MPI
    #include <mpi.h>
#else
    #include "public/mpidummy.h"
#endif

#include "public/ADIOS.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    adios::ADIOS adios;

    try //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        const std::string group( "Types" );
        const std::string numbersVariable( "Numbers" );

        std::vector<int> myVector( 10 );
        std::iota( myVector.begin(), myVector.end(), 1 );

        //testing with CFStream transport
        adios = adios::ADIOS( "fstream.xml", MPI_COMM_WORLD, true ); //debug mode is on
        adios.MonitorGroups( std::cout ); //Dump group info
        adios.Open( group, "helloVector.txt", "w" );
        adios.Write( group, numbersVariable, &myVector );  //Write to helloVector.txt
        adios.Close( group );
    }
    catch( std::bad_alloc& e )
    {
        if( rank == 0 )
        {
            std::cout << "Bad allocation exception, STOPPING PROGRAM\n";
            std::cout << e.what() << "\n";
        }
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
