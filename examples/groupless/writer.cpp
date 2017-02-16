/*
 * writer.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: pnorbert
 */

#include <vector>
#include <iostream>

#include <mpi.h>
#include "ADIOS_OOP.h"


int main( int argc, char* argv [] )
{
    int rank, nproc;
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank);
    MPI_Comm_size( MPI_COMM_WORLD, &nproc);
    const bool adiosDebug = true;

    adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );

    //Application variable
    const unsigned int Nx = 10;
    const int Nparts = rand()%6 + 5; // random size per process, 5..10 each

    std::vector<double> NiceVar( Nx );
    for (int i=0; i < NX; i++)
    {
        NiceVar[i] = rank*NX + (double)i;
    }
    std::vector<float> RaggedVar( Nparts );
    for (int i=0; i < Nparts; i++)
    {
        RaggedVar[i] = rank*NX + (float)i;
    }


    try
    {
        //Define group and variables with transforms, variables don't have functions, only group can access variables
        adios::Variable<int>&    varNX      = adios.DefineVariable<unsigned int>( "NX" ); // global scalar value
        adios::Variable<int>&    varNparts  = adios.DefineVariable<int>( "Nparts" ); // scalar value different on every process
        adios::Variable<double>& varNice    = adios.DefineVariable<double>( "D", {nproc*Nx} ); // 1D global array
        adios::Variable<float>&  varRagged  = adios.DefineVariable<float>( "F", {nproc,adios::VARYING_DIMENSION} ); // local array

        //add transform to variable in group...not executed (just testing API)
        adios::Transform bzip2 = adios::transform::BZIP2( );
        NiceVar.AddTransform( bzip2, 1 );

        //Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method& bpWriterSettings = adios.GetMethod( "output" );
        if (bpWriterSettings.undeclared())
        {
            // if not defined by user, we can change the default settings
            bpWriterSettings.SetEngine( "BP" ); // BP is the default engine
            bpWriterSettings.SetAggregation( (nproc+1)/2 ); // number of aggregators
            bpWriterSettings.AddTransport( "BP", "have_metadata_file=yes" ); // BP is the default transport
        }

        //Open returns a smart pointer to Engine containing the Derived class Writer
        // "w" means we overwrite any existing file on disk, but AdvanceStep will append steps later.
        auto bpWriter = adios.Open( "myNumbers.bp", "w", bpWriterSettings );

        if( bpWriter == nullptr )
            throw std::ios_base::failure( "ERROR: failed to open ADIOS bpWriter\n" );

        if (rank == 0)
        {
            bpWriter->Write<unsigned int>( varNX, &Nx );
        }
        bpWriter->Write<int>( varNparts, &Nparts );

        // Make a 1D selection to describe the local dimensions of the variable we write and
        // its offsets in the global spaces
        adios::Selection& sel = adios.SelectionBoundingBox( {Nx}, {rank*Nx} ); // local dims and offsets; both as list
        NiceVar.SetSelection( sel );
        bpWriter->Write<double>( varMyDoubles, NiceVar.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived

        adios::Selection& lsel = adios.SelectionBoundingBox( {Nx} ); // 0 offsets assumed
        RaggedVar.SetSelection( sel );
        bpWriter->Write<float>( varMyFloats, RaggedVar.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived

        // Indicate we are done for this step
        // N-to-M Aggregation, disk I/O will be performed during this call, unless
        // time aggregation postpones all of that to some later step
        bpWriter->Advance( );

        // Called once: indicate that we are done with this output for the run
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



