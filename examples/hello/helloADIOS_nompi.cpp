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
        adios::ADIOS adios( "writer2Groups.xml", true ); //testing with CPOSIXNoMPI
        adios.MonitorGroups( std::cout );
        std::cout << "Finished initializing ADIOS\n";
    }
    catch( std::exception& e )
    {
        std::cout << e.what() << "\n";
    }

    return 0;
}

