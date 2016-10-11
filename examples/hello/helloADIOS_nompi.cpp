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

        for( auto& groupPair : adiosFile.m_Groups )
        {
            std::cout << "Group " << groupPair.first << "\n";

            for( auto& variablePair : groupPair.second.Variables )
            {
                std::cout << "\t Variable " << variablePair.first << "\tDimensions: " << variablePair.second->DimensionsCSV << "\n";
            }
        }

    }
    catch( std::exception& e )
    {
        std::cout << e.what() << "\n";
    }

    return 0;
}

