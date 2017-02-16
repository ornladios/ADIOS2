/*
 * writer.cpp
 *
 *  Created on: Feb 13, 2017
 *      Author: pnorbert
 */

#include <vector>
#include <iostream>

#include <mpi.h>
#include "ADIOS_CPP.h"

namespace adios {
    typedef enum {
        VARYING_DIMENSION = -1,
        LOCAL_VALUE = 0,
        GLOBAL_VALUE = 1
    };
}

int main( int argc, char* argv [] )
{
    int rank, nproc;
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &rank);
    MPI_Comm_size( MPI_COMM_WORLD, &nproc);
    const bool adiosDebug = true;
    const int NSTEPS = 5;

    adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );

    //Application variable
    const unsigned int Nx = 10;
    int Nparts; // random size per process, 5..10 each

    std::vector<double> NiceArray( Nx );
    for( int i=0; i < Nx; i++ )
    {
        NiceArray[i] = rank*Nx + (double)i;
    }

    std::vector<float> RaggedArray;

    try
    {
        //Define group and variables with transforms, variables don't have functions, only group can access variables
        adios::Variable<unsigned int>& varNX = adios.DefineVariable<unsigned int>( "NX" ); // global single-value across processes
        adios::Variable<int>&    varNproc   = adios.DefineVariable<int>( "nproc", adios::GLOBAL_VALUE ); // same def for global value
        adios::Variable<int>&    varNparts  = adios.DefineVariable<int>( "Nparts", adios::LOCAL_VALUE ); // a single-value different on every process
        adios::Variable<double>& varNice    = adios.DefineVariable<double>( "Nice", {nproc*Nx} ); // 1D global array
        adios::Variable<float>&  varRagged  = adios.DefineVariable<float>( "Ragged", {nproc,adios::VARYING_DIMENSION} ); // ragged array

        //add transform to variable in group...not executed (just testing API)
        adios::Transform bzip2 = adios::transform::BZIP2( );
        varNice->AddTransform( bzip2, 1 );

        //Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method& bpWriterSettings = adios.GetMethod( "output" );
        if( bpWriterSettings.undeclared() )
        {
            // if not defined by user, we can change the default settings
            bpWriterSettings.SetEngine( "BP" ); // BP is the default engine
            bpWriterSettings.AddTransport( "File", "lucky=yes" ); // ISO-POSIX file is the default transport
                                                                  // Passing parameters to the transport
            bpWriterSettings.SetParameters("have_metadata_file","yes" ); // Passing parameters to the engine
            bpWriterSettings.SetParameters( "Aggregation", (nproc+1)/2 ); // number of aggregators
        }

        //Open returns a smart pointer to Engine containing the Derived class Writer
        // "w" means we overwrite any existing file on disk, but AdvanceStep will append steps later.
        auto bpWriter = adios.Open( "myNumbers.bp", "w", bpWriterSettings );

        if( bpWriter == nullptr )
            throw std::ios_base::failure( "ERROR: failed to open ADIOS bpWriter\n" );

        for ( int step; step < NSTEPS; step++ )
        {
            int Nparts = rand()%6 + 5; // random size per process, 5..10 each
            RaggedArray.reserve(Nparts);
            for( int i=0; i < Nparts; i++ )
            {
                RaggedArray[i] = rank*Nx + (float)i;
            }

            if( rank == 0 )
            {
                // Writing a global scalar from only one process
                bpWriter->Write<unsigned int>( varNX, &Nx );
            }
            // Writing a local scalar on every process. Will be shown at reading as a 1D array
            bpWriter->Write<int>( varNparts, &Nparts );

            // Writing a global scalar on every process is useless. Information will be thrown away
            // and only rank 0's data will be in the output
            bpWriter->Write<int>( varNproc, &nproc );

            // Make a 1D selection to describe the local dimensions of the variable we write and
            // its offsets in the global spaces
            adios::Selection& sel = adios.SelectionBoundingBox( {Nx}, {rank*Nx} ); // local dims and offsets; both as list
            NiceArray.SetSelection( sel );
            bpWriter->Write<double>( varNice, NiceArray.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived

            adios::Selection& lsel = adios.SelectionBoundingBox( {1,Nparts}, {rank,0} );
            RaggedArray.SetSelection( sel );
            bpWriter->Write<float>( varRagged, RaggedArray.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived

            // Indicate we are done for this step
            // N-to-M Aggregation, disk I/O will be performed during this call, unless
            // time aggregation postpones all of that to some later step
            bpWriter->Advance( );
        }

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



