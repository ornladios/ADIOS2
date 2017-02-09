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

    //Application variables from fluid and solid
    const unsigned int fluidNx = 3;
    const unsigned int fluidNy = 3;
    const unsigned int fluidNz = 3;
    std::vector<double> fluidTemperature = { 1, 2, 3,   4, 5, 6,   7, 8, 9,
                                             1, 2, 3,   4, 5, 6,   7, 8, 9,
                                             1, 2, 3,   4, 5, 6,   7, 8, 9  };

    const unsigned int solidNx = 5;
    const unsigned int solidNy = 3;
    const unsigned int solidNz = 3;
    std::vector<double> solidTemperature = { 111, 112, 113,    121, 122, 123,    131, 132, 133,
                                             211, 212, 213,    221, 222, 223,    231, 232, 233,
                                             311, 312, 313,    321, 322, 323,    331, 332, 333,
                                             411, 412, 413,    421, 422, 423,    431, 432, 433,
                                             511, 512, 513,    521, 522, 523,    531, 532, 533
                                           };

    try
    {
        //Define fluid group and variables
        adios::Group& ioFluid = adios.DeclareGroup( "fluid" );
        adios::Var ioFluidNx = ioFluid.DefineVariable<unsigned int>( "fluidNx" );
        adios::Var ioFluidNy = ioFluid.DefineVariable<unsigned int>( "fluidNy" );
        adios::Var ioFluidNz = ioFluid.DefineVariable<unsigned int>( "fluidNz" );
        adios::Dims ioFluidDims = ioFluid.SetDimensions( { ioFluidNx, ioFluidNy, ioFluidNz } );
        adios::Var ioFluidTemperature = ioFluid.DefineVariable<double>( "fluidTemperature", ioFluidDims );

        //add transform to variable in group...not executed (just testing API)
        adios::Transform bzip2 = adios::transform::BZIP2( );
        ioFluid.AddTransform( ioFluidTemperature, bzip2 , 1 );

        //adios::Transform merge = adios::transform::Merge( 1 );  //potential merge transform? 1 is a tag
        //ioFluid.AddTransform( ioFluidTemperature, merge , 1 ); //potential merge transform? 1 is a tag

        //Define solid group and variables
        adios::Group& ioSolid = adios.DeclareGroup( "solid" );
        adios::Var ioSolidNx = ioSolid.DefineVariable<unsigned int>( "solidNx" );
        adios::Var ioSolidNy = ioSolid.DefineVariable<unsigned int>( "solidNy" );
        adios::Var ioSolidNz = ioSolid.DefineVariable<unsigned int>( "solidNz" );
        adios::Dims ioSolidDims = ioSolid.SetDimensions( { ioSolidNx, ioSolidNy, ioSolidNz } );
        adios::Var ioSolidTemperature = ioSolid.DefineVariable<double>( "solidTemperature", ioSolidDims );

        //adios::Transform merge = adios::transform::Merge( 1 );  //potential merge transform? 1 is a tag
        //ioSolid.AddTransform( ioSolidTemperature, merge , 1 ); //potential merge transform? 1 is a tag


        //Define method for engine creation
        adios::Method& visSettings = adios.DeclareMethod( "SimpleTask", "Vis" );
        visSettings.AddTransport( "VisIt", "use_shared_memory=no", "hex_mesh=true", "vtk-m_cores=8" ); //as many as you want
        visSettings.AddTransport( "POSIX", "have_metadata_file=yes" );

        //Create engine, smart pointer due to polymorphism
        //Open returns a smart pointer to Engine containing the Derived class Vis engine
        auto visWriter = adios.Open( "visEngine.tmp", "w", visSettings );

        visWriter->SetDefaultGroup( ioFluid ); //default group can change
        visWriter->Write<unsigned int>( ioFluidNx, &fluidNx ); //group, variableName, *value
        visWriter->Write<unsigned int>( ioFluidNy, &fluidNy );
        visWriter->Write<unsigned int>( ioFluidNz, &fluidNz );
        visWriter->Write<double>( ioFluidTemperature, fluidTemperature.data() );

        visWriter->SetDefaultGroup( ioSolid ); //default group can change
        visWriter->Write<unsigned int>( ioSolidNx, &solidNx ); //group, variableName, *value
        visWriter->Write<unsigned int>( ioSolidNy, &solidNy );
        visWriter->Write<unsigned int>( ioSolidNz, &solidNz );
        visWriter->Write<double>( ioSolidTemperature, solidTemperature.data() );

        visWriter->Close( );
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



