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
    std::vector<double> myDoubles;
    std::vector<float> myFloats;
    const unsigned int Nx;

    try
    {

        //Define method for engine creation
        // 1. Get method def from config file or define new one
        adios::Method& bpReaderSettings = adios.GetMethod( "input" );
        if (bpReaderSettings.undeclared())
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

        bpReader->Read<unsigned int>( "Nx", &Nx );

        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void> > varMyDoubles = bpReader.InquiryVariable("myDoubles");

        if( varMyDoubles == nullptr )
            throw std::ios_base::failure( "ERROR: failed to find variable 'myDoubles' in input file\n" );

        // ? how do we know about the type? std::string varMyDoubles->m_Type
        unsigned long long int gdim = varMyDoubles->m_GlobalDimensions[0];
        unsigned long long int ldim = gdim / nproc;
        unsigned long long int offs = rank * ldim;
        if (rank == nproc-1)
        {
            ldim = gdim - (ldim * gdim);
        }

        myDoubles.resize(ldim);

        // Make a 1D selection to describe the local dimensions of the variable we READ and
        // its offsets in the global spaces
        adios::Selection& sel = adios.SelectionBoundingBox( {ldim}, {offs} ); // local dims and offsets; both as list
        bpReader->Read<double>( "myDoubles", myDoubles.data() ); // Base class Engine own the Read<T> that will call overloaded Read from Derived


        // inquiry about a variable, whose name we know
        std::shared_ptr<adios::Variable<void> > varMyFloats = bpReader.InquiryVariable("myFloats");
        // We have here varMyFloats->sum_nblocks, nsteps, nblocks[], global
        if (rank < varMyFloats->nblocks[0])
        {
            // get per-writer size information
            varMyFloats->InquiryBlocks();
            // now we have the dimensions per block

            unsigned long long int ldim = varMyFloats->blockinfo[rank].m_Dimensions[0];
            myFloats.resize( ldim );

            adios::Selection& sel = adios.SelectionWriteblock( rank );
            bpReader->Read<double>( "myDoubles", myDoubles.data() );
        }

        // promise to not read more from this step
        bpReader->ReleaseStep();

        // want to move on to the next available step
        //bpReader->AdvanceStep(adios::NextImmediateStep);
        //bpReader->AdvanceStep(adios::NextAvailableStep);
        bpReader->AdvanceStep(); // default is adios::NextAvailableStep


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



