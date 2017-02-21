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

    try
    {
        //Define method for engine creation, it is basically straight-forward parameters
        adios::Method& datamanSettings = adios.DeclareMethod( "WAN", "DataManReader" ); //default method type is BPWriter
        datamanSettings.SetParameters( "peer-to-peer=yes" );
        datamanSettings.AddTransport( "Mdtm", "localIP=128.0.0.0.1", "remoteIP=128.0.0.0.2", "tolerances=1,2,3" );
        //datamanSettings.AddTransport( "ZeroMQ", "localIP=128.0.0.0.1.1", "remoteIP=128.0.0.0.2.1", "tolerances=1,2,3" ); not yet supported , will throw an exception

        //Create engine smart pointer to DataManReader Engine due to polymorphism,
        //Open returns a smart pointer to Engine containing the Derived class DataManReader
        auto datamanReader = adios.Open( "myDoubles.bp", "r", datamanSettings );

        if( datamanReader == nullptr )
            throw std::ios_base::failure( "ERROR: failed to create DataMan I/O engine at Open\n" );

        adios::Variable<double>* ioMyDoubles = datamanReader->InquireVariableDouble( "ioMyDoubles" );
        if( ioMyDoubles == nullptr )
            std::cout << "Variable ioMyDoubles not read...yet\n";

        datamanReader->Close( );
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



