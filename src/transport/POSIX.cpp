/*
 * POSIXMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */


#include <stdio.h>

#include "transport/POSIX.h"
#include "functions/adiosFunctions.h" // CreateDirectory


namespace adios
{


POSIX::POSIX( MPI_Comm mpiComm, const bool debugMode, const std::vector<std::string>& arguments ):
    Transport( "POSIX", mpiComm, debugMode ),
    m_File( NULL )
{ }


POSIX::~POSIX( )
{ }


void POSIX::Open( const std::string streamName, const std::string accessMode )
{
    const std::string directory( streamName + ".dir" );

    //data.bp.dir
    if( m_MPIRank == 0 )
        CreateDirectory( directory );

    MPI_Barrier( m_MPIComm ); //all processor wait until directory is created

    const std::string streamNameRank( directory + "/" + streamName + "." + std::to_string( m_MPIRank ) );

    if( accessMode == "w" || accessMode == "write" )
        m_File = fopen( streamNameRank.c_str(), "w" );

    else if( accessMode == "a" || accessMode == "append" )
        m_File = fopen( streamNameRank.c_str(), "a" );

    else if( accessMode == "r" || accessMode == "read" )
        m_File = fopen( streamNameRank.c_str(), "r" );

    if( m_DebugMode == true )
    {
        if( m_File == NULL )
            throw std::ios_base::failure( "ERROR: couldn't open file " + streamName + " in Open function of POSIX transport\n" );
    }

    MPI_Barrier( m_MPIComm ); //all of them must wait until the file is opened
}


void POSIX::SetBuffer( std::vector<char>& buffer )
{
    int status = setvbuf( m_File, &buffer[0], _IOFBF, buffer.size() );

    if( m_DebugMode == true )
    {
        if( status == 1 )
            throw std::ios_base::failure( "ERROR: could not set buffer in rank " + std::to_string( m_MPIRank ) + "\n" );
    }
}


void POSIX::Write( std::vector<char>& buffer )
{
    fwrite( &buffer[0], sizeof(char), buffer.size(), m_File );
}


void POSIX::Close( )
{
    fclose( m_File );
}


//PRIVATE FUNCTIONS
void POSIX::Init( const std::vector<std::string>& arguments )
{

}



}//end namespace
