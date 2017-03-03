/*
 * FD.cpp file descriptor
 *
 *  Created on: Oct 6, 2016
 *      Author: wfg
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <fcntl.h>  //open
#include <sys/types.h> //open
#include <sys/stat.h> //open
#include <stddef.h> // write output
#include <unistd.h> // write, close
#include <ios> //std::ios_base::failure
/// \endcond


#include "transport/file/FD.h"


namespace adios
{
namespace transport
{

FD::FD( MPI_Comm mpiComm, const bool debugMode ):
    Transport( "FD", mpiComm, debugMode )
{ }


FD::~FD( )
{
    if( m_FileDescriptor != -1 )
    {
        close( m_FileDescriptor );
    }
}


void FD::Open( const std::string name, const std::string accessMode )
{
    m_Name = name;
    m_AccessMode = accessMode;

    if( accessMode == "w" || accessMode == "write" )
    {
        m_FileDescriptor = open( m_Name.c_str(), O_WRONLY | O_CREAT, 0777 );
    }
    else if( accessMode == "a" || accessMode == "append" )
    {
        m_FileDescriptor = open( m_Name.c_str(),  O_WRONLY | O_APPEND );
    }
    else if( accessMode == "r" || accessMode == "read" )
    {
        m_FileDescriptor = open( m_Name.c_str(), O_RDONLY );
    }

    if( m_DebugMode == true )
    {
        if( m_FileDescriptor == -1 )
            throw std::ios_base::failure( "ERROR: couldn't open file " + m_Name +
                                          ", from call to Open in FD transport using POSIX Open\n" );
    }
}


void FD::Write( const char* buffer, std::size_t size )
{
    auto writtenSize = write( m_FileDescriptor, buffer, size );

    if( m_DebugMode == true )
    {
        if( writtenSize == -1 )
            throw std::ios_base::failure( "ERROR: couldn't write to file " + m_Name +
                                          ", in call to POSIX write\n"   );

        if( static_cast<std::size_t>( writtenSize ) != size )
            throw std::ios_base::failure( "ERROR: written size + " + std::to_string( writtenSize ) +
                                          " is not equal to intended size " +
                                          std::to_string( size ) + " in file " + m_Name +
                                          ", in call to POSIX write\n"   );
    }
}


void FD::Close( )
{
    int status = close( m_FileDescriptor );

    if( m_DebugMode == true )
    {
        if( status == -1 )
            throw std::ios_base::failure( "ERROR: couldn't close file " + m_Name +
                                          ", in call to POSIX write\n"   );
    }
}


} //end namespace transport
}//end namespace
