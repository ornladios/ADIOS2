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
    std::vector<int> myInts = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int myIntsSize = 10;

    try
    {
        //Define group and variables
        adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );
        const std::string groupName( "ints" );
        adios.DeclareGroup( groupName );
        adios.DefineVariable( groupName, "myIntsSize", "int" );
        adios.DefineVariable( groupName, "myInts", "double", "myIntsSize" );

        //Define method...
        const std::string methodName( "DataManSend" );
        adios.DeclareMethod( methodName, "DataMan" ); //2nd item is type and must be supported e.g. Writer (empty default), DataMan, Sirius, etc.
//        adios.AddTransport( methodName, "ZeroMQ", "format=json", "tcp=128.11.1.1.2", "real_time=yes" );
//        adios.AddTransport( methodName, "MDTM", "format=otherFormat", "tcp=128.11.1.1.2" );
//        adios.AddTransport( methodName, "POSIX", "fname=myfile.bp" ); //you can write things to file as well


        //this illustrates method uniqueness
        const std::string methodName2( "DataManSend2" );
        adios.DeclareMethod( methodName2, "DataMan" ); //should this be variadic?
        adios.AddTransport( methodName2, "ZeroMQ", "format=json", "tcp=128.11.1.1.2", "realtime_change=yes" );


        //Create engine handler and Write
        int handler = adios.Open( "myInts.bp", "w", methodName );
        adios.SetDefaultGroup( handler, groupName );

        double varDouble = 10.;
        adios.Write<double>( handler, "myIntsSize", &varDouble );
        adios.Write<int>( handler, "myInts", &myInts.front() );
        adios.Close( handler );

        int handler2 = adios.Open( "somethingelse.bp", "w", methodName2 );
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



