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
    #include "../../include/mpidummy.h"
#endif

#include "../../include/ADIOS.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    try
    {
        unsigned int size = 10;
        std::vector<int> numbers( size, 1 );

        adios::ADIOS adios( "numbers.xml", MPI_COMM_WORLD, true ); //xml file, MPI Comm, debug mode
        adios.Open( "numbers.bp", "write", "FStream" );// open a file stream transport for writing to file numbers.bp
        adios.SetCurrentGroup( "numbers.bp", "Vector" );// Write will look for variables in the Vector group defined in numbers.xml
        adios.Write( "numbers.bp", "size", &size ); //writes size
        adios.Write( "numbers.bp", "numbers", &numbers[0] ); //writes vector numbers
        adios.Close( "numbers.bp" ); //flush and closes numbers.bp now containing size and numbers
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
