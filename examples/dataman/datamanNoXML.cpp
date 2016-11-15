/*
 * dataman.cpp: Example for DataMan Transport usage
 *
 *  Created on: Nov 15, 2016
 *      Author: wfg
 */




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

    try //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        const unsigned int myCharsSize = 10;
        std::vector<char> myChars( myCharsSize, '1' ); // 10 chars with value '1'

        //Equivalent to adios_init debug mode is on, MPI_COMM_WORLD is dummy nothing to worry about
        adios::ADIOS adios( MPI_COMM_WORLD, true );

        //Create group TCP and set up, this can be done from XML config file
        const std::string group( "TCP" );
        adios.CreateGroup( group );
        adios.CreateVariable( group, "myCharsSize", "unsigned int" ); //scalar : group, name, type
        adios.CreateVariable( group, "myChars",     "char",         "myCharsSize" ); //group, name, type, integer variable defining size
        //here we tell group to be associate with a DataMan transport
        //we can add more parameters if you require
        adios.SetTransport( group, "DataMan" );

        adios.Open( group, "TCPStream", "write" ); //here open a stream called TCPStream for writing (w or write)
        adios.Write( group, "myCharsSize", &myCharsSize ); //calls your transport
        adios.Write( group, "myChars"    , &myChars[0] ); //calls your transport
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


    return 0;
}



