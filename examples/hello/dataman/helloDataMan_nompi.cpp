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
        //Define variable and local size
        auto& ioMyDoubles = adios.DefineVariable<double>( "myDoubles", {Nx} );

        //Define method for engine creation, it is basically straight-forward parameters
        adios::Method& datamanSettings = adios.DeclareMethod( "WAN", "DataMan" ); //default method type is Writer
        datamanSettings.SetParameters( "peer-to-peer=yes", "optimize=yes", "compress=yes" );
        datamanSettings.AddTransport( "Mdtm", "localIP=128.0.0.0.1", "remoteIP=128.0.0.0.2", "tolerances=1,2,3" );
        //datamanSettings.AddTransport( "ZeroMQ", "localIP=128.0.0.0.1.1", "remoteIP=128.0.0.0.2.1", "tolerances=1,2,3" ); not yet supported , will throw an exception

        //Create engine smart pointer to DataMan Engine due to polymorphism,
        //Open returns a smart pointer to Engine containing the Derived class DataMan
        auto datamanWriter = adios.Open( "myDoubles.bp", "w", datamanSettings );

        if( datamanWriter == nullptr )
            throw std::ios_base::failure( "ERROR: failed to create DataMan I/O engine at Open\n" );

        //datamanWriter->Write( "myDoubles", myDoubles.data() ); //you can write either by string or by object
        datamanWriter->Write( ioMyDoubles, myDoubles.data() ); // Base class Engine own the Write<T> that will call overloaded Write from Derived
        datamanWriter->Close( );
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



