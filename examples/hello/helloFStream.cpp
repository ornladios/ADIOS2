/*
 * helloFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */




#include <stdexcept>
#include <iostream>
#include <fstream>
#include <string>
#include <numeric>

#include "public/ADIOS.h"


int main( int argc, char* argv [] )
{
    try
    {
        const std::string vectorGroup("Vector");
        const std::string numbersVariable("Numbers");

        std::vector<int> myVector(10);
        std::iota( myVector.begin(), myVector.end(), 1 );

        //testing with CPOSIXMPI
        adios::ADIOS adios( "fstream.xml" );
        adios.MonitorGroups( std::cout ); //Get Monitor info

        adios.Write( vectorGroup, numbersVariable, &myVector[0] );  //Write

    }
    catch( std::exception& e ) //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        std::cout << e.what() << "\n";
    }

    return 0;
}
