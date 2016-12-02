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


CDataMan::CDataMan(  MPI_Comm mpiComm, const bool debugMode, const std::vector<std::string>& arguments ):
    CTransport( "DataMan", mpiComm, debugMode )
{
    Init( arguments );
}


CDataMan::~CDataMan( )
{ }


void CDataMan::Open( const std::string streamName, const std::string accessMode )
{
    if( accessMode == "w" || accessMode == "write" )
    {
        //here open your socket and assign it to this streamName;
    }
}


void CDataMan::SetBuffer( std::vector<char>& buffer )
{
    //empty for now
}


void CDataMan::Write( std::vector<char>& buffer )
{
    //here comes your magic, expect buffer to contain the raw data from the ADIOS.Write variable using memcpy
    //for now it's a reference, if later it goes out of scope we can move buffer here
}


void CDataMan::Close( )
{
    //close any hanging resources from your transport
}

//PRIVATE FUNCTIONS
void CDataMan::Init( const std::vector<std::string>& arguments )
{

}


} //end namespace
