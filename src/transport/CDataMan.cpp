/*
 * CDataMan.cpp Implementation of ./include/transport/CDataMan.h
 *
 *  Created on: Nov 15, 2016
 *      Author: wfg
 */

#include "transport/CDataMan.h"

//here include any external library headers, we can figure out linking them later


namespace adios
{


CDataMan::CDataMan(  MPI_Comm mpiComm, const bool debugMode ):
    CTransport( "DataMan", mpiComm, debugMode )
{ }


CDataMan::~CDataMan( )
{ }


void CDataMan::Open( const std::string streamName, const std::string accessMode )
{
    if( accessMode == "w" || accessMode == "write" )
    {
        //here open your socket and assign it to this streamName;
    }
}


void CDataMan::SetBuffer( const std::vector<char>& buffer )
{
    //empty for now
}


void CDataMan::Write( std::vector<char>& buffer )
{
    //here comes your magic, expect buffer to contain the raw data (using memcpy or insert) from the ADIOS.Write variable
    //for now it's a reference, if later it goes out of scope we can move buffer here
}


void CDataMan::Close( )
{
    //close any hanging resources from your transport
}


} //end namespace
