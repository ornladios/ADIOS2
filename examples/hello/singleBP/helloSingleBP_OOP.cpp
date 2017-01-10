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
        adios::Method method; //( "SingleBP", adiosDebug );
        method.AddCapsule( "Heap" );
        method.AddTransport( "POSIX", "have_metadata_file=0" );

        //Create engine and Write
        adios::engine::SingleBP singleBP( "myInts.bp", "w", MPI_COMM_WORLD, method, adiosDebug );
        singleBP.SetDefaultGroup( group );
        singleBP.Write( "myIntsSize", &myIntsSize  );
        singleBP.Write( "myInts", &myInts.front() );
        singleBP.Close( );
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



