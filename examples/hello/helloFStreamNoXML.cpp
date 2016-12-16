/*
/*
 * helloFStreamNoXML.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: wfg
 */

#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>

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

        adios::ADIOS adios( MPI_COMM_WORLD, true ); //MPI Comm, debug mode

        const std::string group( "Vector" );
        adios.DeclareGroup( group );
        adios.DefineVariable( group, "size", "unsigned int" );
        adios.DefineVariable( group, "numbers", "int", "size" );
        adios.DefineAttribute( group, "description", "string", "1 to 10" );

        const std::string file( "numbers.bp" );
        adios.Open( file, "write", "FStream" );// open a file stream transport for writing to file numbers.bp
        adios.SetCurrentGroup( file, group );// Write will look for variables in the Vector group defined in numbers.xml
        adios.Write( file, "size", &size ); //writes size
        adios.Write( file, "numbers", &numbers[0] ); //writes vector numbers
        adios.Close( file ); //flush and closes numbers.bp now containing size and numbers
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
 * helloFStreamNoXML.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: wfg
 */




