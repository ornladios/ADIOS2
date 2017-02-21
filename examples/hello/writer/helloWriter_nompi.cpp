/*
 * helloADIOSNoXML_OOP.cpp
 *
 *  Created on: Jan 9, 2017
 *      Author: wfg
 */

#include <vector>
#include <iostream>

#include "ADIOS_CPP.h"


int main( int argc, char* argv [] )
{
    const bool adiosDebug = true;
    adios::ADIOS adios( adiosDebug );

    //Application variable
    std::vector<double> myDoubles = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    const std::size_t Nx = myDoubles.size();

    try
    {
        //Define variable and local size
        auto& ioMyDoubles = adios.DefineVariable<double>( "myDoubles", adios::Dims{Nx} );

        //Define method for engine creation, it is basically straight-forward parameters
        adios::Method& bpWriterSettings = adios.DeclareMethod( "SinglePOSIXFile" ); //default method type is Writer
        bpWriterSettings.AddTransport( "POSIX", "have_metadata_file=yes" );

        //Create engine smart pointer due to polymorphism,
        //Open returns a smart pointer to Engine containing the Derived class Writer
        auto bpWriter = adios.Open( "myDoubles_nompi.bp", "w", bpWriterSettings );

        if( bpWriter == nullptr )
            throw std::ios_base::failure( "ERROR: couldn't create bpWriter at Open\n" );

        bpWriter->Write<double>( ioMyDoubles, myDoubles.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived
        bpWriter->Close( );
    }
    catch( std::invalid_argument& e )
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch( std::ios_base::failure& e )
    {
        std::cout << "System exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    return 0;
}



