/*
 * helloADIOS_nompi.cpp
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */


#include <stdexcept>
#include <iostream>

#include "public/ADIOS.h"


int main( int argc, char* argv [] )
{
    try
    {
        adios::ADIOS adiosFile( "writer2Groups.xml" ); //testing with CPOSIXNoMPI
        adiosFile.Init( );
        adiosFile.MonitorGroups( std::cout );
        std::cout << "Finished initializing ADIOS\n";
    }
    catch( std::exception& e )
    {
        std::cout << e.what() << "\n";
    }

    return 0;
}

