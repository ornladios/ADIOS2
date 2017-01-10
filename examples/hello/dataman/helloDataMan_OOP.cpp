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


#include "ADIOS_OOP.h"


int main( int argc, char* argv [] )
{
    MPI_Init( &argc, &argv );
    int rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    const bool adiosDebug = true;

    //Application variable
    std::vector<double> myInts = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int myIntsSize = static_cast<int>( myInts.size() );

    try
    {
        //Define group and variables
        adios::Group group( adiosDebug );
        group.DefineVariable( "myIntsSize", "int" ); //define size as scalar
        group.DefineVariable( "myInts",     "double", "myIntsSize" ); //define variable with associate size

        //Define method
        adios::Method method( "DataMan", adiosDebug );
        method.AddCapsule( "Heap" );
        method.AddTransport( "POSIX", "have_metadata_file=0" ); //option to save to file
        method.AddTransport( "TCP_IP", "fname=myfile.tcp" ); //here you can add as many options as you want in the format "parameter=value"

        //Create engine and Write
        adios::engine::DataMan dataMan( "myMessage", "w", MPI_COMM_WORLD, method, adiosDebug );
        dataMan.SetDefaultGroup( group );
        dataMan.Write( "myIntsSize", &myIntsSize  ); //issue hello
        dataMan.Write( "myInts", &myInts.front() ); //issue hello
        dataMan.Close( );
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



