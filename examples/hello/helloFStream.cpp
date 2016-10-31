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
    try //need to think carefully how to handle C++ exceptions with MPI to avoid deadlocking
    {
        const std::string group( "Types" );
        const std::string numbersVariable( "Numbers" );

        std::vector<int> myVector( 10 );
        std::iota( myVector.begin(), myVector.end(), 1 );

        //testing with CFStream transport
        adios::ADIOS adios( "fstream.xml", true );
        adios.MonitorGroups( std::cout ); //Get Monitor info
        adios.Write( group, numbersVariable, &myVector );  //Write
        adios.Close( group1 );

        adios.Write( group2, numbersVariable, &myVector );  //Write


        adios.CloseAll( );

    }
    catch( std::bad_alloc& e )
    {
        std::cout << "Bad allocation exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch( std::invalid_argument& e )
    {
        std::cout << "Invalid argument exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }
    catch( std::exception& e )
    {
        std::cout << "Exception, STOPPING PROGRAM\n";
        std::cout << e.what() << "\n";
    }

    return 0;
}
