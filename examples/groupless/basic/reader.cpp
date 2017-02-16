/*
 * reader.cpp
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
    MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    MPI_Comm_size( MPI_COMM_WORLD, &nproc);
    const bool adiosDebug = true;

    adios::ADIOS adios( MPI_COMM_WORLD, adiosDebug );

    //Application variable
    std::vector<double> NiceArray;
    std::vector<float> RaggedArray;
    unsigned int Nx;
    int Nparts;
    int Nwriters;

    try
    {
        //Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method& bpReaderSettings = adios.GetMethod( "input" );
        if( bpReaderSettings.undeclared() )
        {
            // if not defined by user, we can change the default settings
            bpReaderSettings.SetEngine( "BP" ); // BP is the default engine
        }

        //Create engine smart pointer due to polymorphism,
        // Default behavior
        // auto bpReader = adios.Open( "myNumbers.bp", "r" );
        // this would just open with a default transport, which is "BP"
        auto bpReader = adios.Open( "myNumbers.bp", "r", bpReaderSettings );

        if( bpReader == nullptr )
            throw std::ios_base::failure( "ERROR: failed to open ADIOS bpReader\n" );

        /* NX */
        bpReader->Read<unsigned int>( "NX", &Nx );  // read a Global scalar which has a single value in a step

        /* nproc */
        bpReader->Read<int>( "nproc", &Nwriters );  // also a global scalar


        /* Nparts */
        // Nparts local scalar is presented as a 1D array of Nwriters elements.
        // We need to read a specific value the same way as reading from any 1D array.
        // Make a single-value selection to describe our rank's position in the
        // 1D array of Nwriters values.
        if( rank < Nwriters )
        {
            std::unique_ptr<adios::Selection> selNparts = adios.SelectionBoundingBox( {1}, {rank} );
            bpReader->Read<int>( "Nparts", selNparts, &Nparts );
        }


        /* Nice */
        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void> > varNice = bpReader.InquiryVariable("Nice");

        if( varNice == nullptr )
            throw std::ios_base::failure( "ERROR: failed to find variable 'myDoubles' in input file\n" );

        // ? how do we know about the type? std::string varNice->m_Type
        unsigned long long int gdim = varMyDoubles->m_GlobalDimensions[0];  // ?member var or member func?
        unsigned long long int ldim = gdim / nproc;
        unsigned long long int offs = rank * ldim;
        if( rank == nproc-1 )
        {
            ldim = gdim - (ldim * gdim);
        }

        NiceArray.reserve(ldim);

        // Make a 1D selection to describe the local dimensions of the variable we READ and
        // its offsets in the global spaces
        std::unique_ptr<adios::Selection> bbsel = adios.SelectionBoundingBox( {ldim}, {offs} ); // local dims and offsets; both as list
        bpReader->Read<double>( "Nice", bbsel, NiceArray.data() ); // Base class Engine own the Read<T> that will call overloaded Read from Derived



        /* Ragged */
        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void> > varRagged = bpReader.InquiryVariable("Ragged");
        if( varRagged->m_GlobalDimensions[1] != adios::VARYING_DIMENSION)
        {
            throw std::ios_base::failure( "Unexpected condition: Ragged array's fast dimension "
                    "is supposed to be VARYING_DIMENSION\n" );
        }
        // We have here varRagged->sum_nblocks, nsteps, nblocks[], global
        if( rank < varRagged->nblocks[0] ) // same as rank < Nwriters in this example
        {
            // get per-writer size information
            varRagged->InquiryBlocks();
            // now we have the dimensions per block

            unsigned long long int ldim = varRagged->blockinfo[rank].m_Dimensions[0];
            RaggedArray.resize( ldim );

            std::unique_ptr<adios::Selection> wbsel = adios.SelectionWriteblock( rank );
            bpReader->Read<float>( "Ragged", wbsel, RaggedArray.data() );

            // We can use bounding box selection as well
            std::unique_ptr<adios::Selection> rbbsel = adios.SelectionBoundingBox( {1,ldim}, {rank,0} );
            bpReader->Read<float>( "Ragged", rbbsel, RaggedArray.data() );
        }

        /* Extra help to process Ragged */
        int maxRaggedDim = varRagged->m_GlobalDimensions[1]; // contains the largest



        // promise to not read more from this step
        bpReader->Release();

        // want to move on to the next available step
        //bpReader->AdvanceStep(adios::NextImmediateStep);
        //bpReader->AdvanceStep(adios::NextAvailableStep);
        bpReader->Advance(); // default is adios::NextAvailableStep


        // Close file/stream
        bpReader->Close( );
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



