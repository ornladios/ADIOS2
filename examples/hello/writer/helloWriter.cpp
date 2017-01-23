/*
 * helloADIOSNoXML_OOP.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: wfg
 */

#include <vector>
#include <iostream>

#ifdef HAVE_MPI
    #include <mpi.h>
#else
    #include "mpidummy.h"
    using adios::MPI_Init;
    using adios::MPI_Comm_rank;
    using adios::MPI_Finalize;
#endif


#include "ADIOS.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    const bool adiosDebug = true;

    //Application variable
    std::vector<double> myInts = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int myIntsSize = 10;

    try
    {
        //Define group and variables
        adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );
        const std::string groupName( "ints" );
        adios.DeclareGroup( groupName );
        adios.DefineVariable( groupName, "myIntsSize", "int" );
        adios.DefineVariable( groupName, "myInts", "double", "myIntsSize" );

        //Define method
        const std::string methodName( "IntsWriter" );
        adios.DeclareMethod( methodName, "Writer" );
        adios.AddTransport( methodName, "POSIX" );

        //Create engine handler and Write
        int handler = adios.Open( "myInts.bp", "w", methodName );
        adios.SetDefaultGroup( handler, groupName );
        adios.Write<int>( handler, "myIntsSize", &myIntsSize );
        adios.Write<double>( handler, "myInts", &myInts.front() );
        adios.Close( handler );
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



