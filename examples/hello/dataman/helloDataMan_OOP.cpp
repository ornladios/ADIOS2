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
    adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );

    //Application variable
    std::vector<double> myNumbers = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int myNX = static_cast<int>( myNumbers.size() );

    try
    {
        //Define group and variables
        adios::Group& wanGroup = adios.DeclareGroup( "WAN_Group" );
        adios::Var ioNX = wanGroup.DefineVariable<int>( "myNX" );
        adios::Dims ioDim1D = wanGroup.SetDimensions( {ioNX} ); //can be extended to many dimensions {ioNx, ioNy}
        adios::Var ioNumbers = wanGroup.DefineVariable<double>( "myNumbers", ioDim1D );

        adios::Method& wanMethod = adios.DeclareMethod( "WAN_Method", "DataMan" ); //name and type
        wanMethod.AddTransport( "Mdtm", "localIP=128.0.0.0.1", "remoteIP=128.0.0.0.2", "tolerances=1,2,3" );//add as many as you want
        //wanMethod.AddTransport( "POSIX", "have_metadata_file=true" );
        wanMethod.SetDefaultGroup( wanGroup );

        auto dataManWriter = adios.Open( "hello_dataman", "w", wanMethod ); //here pass the method to your engine
        dataManWriter->Write<int>( ioDim1D, &myNX ); //a template Write is good to have
        dataManWriter->Write<double>( ioNumbers, myNumbers.data() );
        dataManWriter->Close( );
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



