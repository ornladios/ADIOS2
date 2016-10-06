/*
 * helloADIOS_nompi.cpp
 *
 *  Created on: Oct 4, 2016
 *      Author: wfg
 */


#include <stdexcept>
#include <iostream>

#include "ADIOS.h"


int main( int argc, char* argv [] )
{
    try
    {
        adios::ADIOS adiosFile( "writer.xml" ); //testing with CPOSIXNoMPI
        adiosFile.Init( );
    }
    catch( std::exception& e )
    {
        std::cout << e.what() << "\n";
    }

    return 0;
}

