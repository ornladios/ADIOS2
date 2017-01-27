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
    std::vector<double> myDoubles = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::vector<float> myFloats = { 0, -1, -2, -3, -4, -5, -6, -7, -8, -9 };
    const unsigned int Nx = 10; //static_cast<unsigned int>( myDoubles.size() );

    try
    {
        //Define group and variables with transforms, variables don't have functions, only group can access variables
        adios::Group& ioGroup = adios.DeclareGroup( "ioGroup" );
        adios::Var ioNx = ioGroup.DefineVariable<unsigned int>( "Nx" );
        adios::Var ioMyDoubles = ioGroup.DefineVariable<double>( "myDoubles", "Nx" );
        adios::Var ioMyFloats = ioGroup.DefineVariable<float>( "myFloats", "Nx" );

        //add transform to variable in group...not executed (just testing API)
        adios::Transform bzip2 = adios::transform::BZIP2( );
        ioGroup.AddTransform( ioMyDoubles, bzip2, 1 );
        ioGroup.AddTransform( ioMyFloats, bzip2, 1 );

        //Define method for engine creation, it is basically straight-forward parameters
        adios::Method& bpWriterSettings = adios.DeclareMethod( "SinglePOSIXFile" ); //default method type is Writer
        bpWriterSettings.AddTransport( "POSIX", "have_metadata_file=yes" );
        bpWriterSettings.SetDefaultGroup( ioGroup );

        //Create engine smart pointer due to polymorphism,
        //Open returns a smart pointer to Engine containing the Derived class Writer
        auto bpWriter = adios.Open( "myNumbers.bp", "w", bpWriterSettings );

        if( bpWriter == nullptr )
            throw std::ios_base::failure( "ERROR: failed to open ADIOS bpWriter\n" );

        bpWriter->Write<unsigned int>( ioNx, &Nx );
        bpWriter->Write<double>( ioMyDoubles, myDoubles.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived
        bpWriter->Write<float>( ioMyFloats, myFloats.data() );
        bpWriter->Close( );
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



