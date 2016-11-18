/*
 * CPOSIXMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

#include <stdio.h>

#include "transport/CPOSIX.h"


namespace adios
{


CPOSIX::CPOSIX( MPI_Comm mpiComm, const bool debugMode ):
    CTransport( "POSIX", mpiComm, debugMode ),
    m_File( NULL )
{ }


CPOSIX::~CPOSIX( )
{ }


void CPOSIX::Open( const std::string streamName, const std::string accessMode )
{
    if( m_RankMPI == 0 )
    {
        if( accessMode == "w" || accessMode == "write" )
            m_File = fopen( streamName.c_str(), "w" );

        else if( accessMode == "a" || accessMode == "append" )
            m_File = fopen( streamName.c_str(), "a" );

        else if( accessMode == "r" || accessMode == "read" )
            m_File = fopen( streamName.c_str(), "r" );

        if( m_DebugMode == true )
        {
            if( m_File == NULL )
                throw std::ios_base::failure( "ERROR: couldn't open file " + streamName + " in Open function\n" );
        }
    }

    MPI_Barrier( m_MPIComm ); //all of them must wait until the file is opened
}


void CPOSIX::SetBuffer( std::vector<char>& buffer )
{
    setvbuf( m_File, &buffer[0], _IOFBF, buffer.size() );
}


void CPOSIX::Close( )
{
    if( m_RankMPI == 0 )
        fclose( m_File );
}



}//end namespace
