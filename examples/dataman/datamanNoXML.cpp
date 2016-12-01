/*
 * datamanNoXML.cpp: Example for DataMan Transport usage also using POSIX as additional transport
 *
 *  Created on: Nov 15, 2016
 *      Author: wfg
 */

#ifdef HAVE_MPI
    #include <mpi.h>
#else
    #include "public/mpidummy.h"
    using adios::MPI_Init;
    using adios::MPI_Comm_rank;
#endif

#include "public/ADIOS.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );

    try //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        //APP variables
        const unsigned int myCharsSize = 10;
        std::vector<char> myChars( myCharsSize, '1' ); // 10 chars with value '1'

        //ADIOS init here, non-XML, debug mode is ON
        adios::ADIOS adios( MPI_COMM_WORLD, true );

        //Create group TCP and define variables
        const std::string groupTCP( "TCP" );
        adios.DeclareGroup( groupTCP );
        adios.DefineVariable( groupTCP, "myCharsSize", "unsigned int" ); //scalar : group, name, type
        adios.DefineVariable( groupTCP, "myChars",     "char",  "myCharsSize" ); //group, name, type, integer variables defining dimensions

        //Open stream using two transports, DataMan is default, POSIX is an additional one
        const std::string streamTCP( "TCP" );
        adios.Open( streamTCP, "write", "DataMan" ); //here open a stream called TCPStream for writing (w or write), name is the same as stream
        adios.AddTransport( streamTCP, "write", "POSIX", "name=TCP.bp" ); //add POSIX transport with .bp name

        //Writing
        adios.SetCurrentGroup( streamTCP, groupTCP ); //no need to add group field in Write
        adios.Write( streamTCP, "myCharsSize", &myCharsSize );
        adios.Write( streamTCP, "myChars", &myChars );
        //Close
        adios.Close( streamTCP ); // Flush to all transports, DataMan and POSIX
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



