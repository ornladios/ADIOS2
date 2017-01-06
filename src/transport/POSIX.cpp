/*
 * POSIXMPI.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "transport/POSIX.h"


namespace adios
{


POSIX::POSIX( MPI_Comm mpiComm, const bool debugMode ):
    Transport( "POSIX", mpiComm, debugMode )
{ }


POSIX::~POSIX( )
{ }


void POSIX::Open( const std::string name, const std::string accessMode )
{
    m_Name = name;
    m_AccessMode = accessMode;

    if( accessMode == "w" || accessMode == "write" )
        m_FileDescriptor = open( m_Name.c_str(), O_WRONLY | O_CREAT, 0666 );

    else if( accessMode == "a" || accessMode == "append" )
        m_FileDescriptor = open( m_Name.c_str(),  O_WRONLY | O_APPEND );

    else if( accessMode == "r" || accessMode == "read" )
        m_FileDescriptor = open( m_Name.c_str(), O_RDONLY );

    if( m_DebugMode == true )
    {
        if( m_FileDescriptor == -1 )
            throw std::ios_base::failure( "ERROR: couldn't open file " + m_Name +
                                          ", from call to Open in POSIX transport\n" );
    }
}


void POSIX::Write( const char* buffer, std::size_t size )
{
    int status = write( m_FileDescriptor, buffer, size );

    if( m_DebugMode == true )
    {
        if( status == -1 )
            throw std::ios_base::failure( "ERROR: couldn't write to file " + m_Name +
                                          ", in call to POSIX write\n"   );
    }
}


void POSIX::Close( )
{
    int status = close( m_FileDescriptor );

    if( m_DebugMode == true )
    {
        if( status == -1 )
            throw std::ios_base::failure( "ERROR: couldn't close file " + m_Name +
                                          ", in call to POSIX write\n"   );
    }
}





}//end namespace
