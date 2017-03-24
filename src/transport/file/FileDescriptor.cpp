/*
 * FileDescriptor.cpp file descriptor
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



#include "transport/file/FileDescriptor.h"


namespace adios
{
namespace transport
{

FileDescriptor::FileDescriptor( MPI_Comm mpiComm, const bool debugMode ):
    Transport( "POSIX_IO", mpiComm, debugMode )
{ }


FileDescriptor::~FileDescriptor( )
{
    if( m_FileDescriptor != -1 )
    {
        close( m_FileDescriptor );
    }
}


void FileDescriptor::Open( const std::string name, const std::string accessMode )
{
    m_Name = name;
    m_AccessMode = accessMode;

    if( accessMode == "w" || accessMode == "write" )
    {
        if( m_Profiler.m_IsActive == true )
            m_Profiler.m_Timers[0].SetInitialTime();

        m_FileDescriptor = open( m_Name.c_str(), O_WRONLY | O_CREAT, 0777 );

        if( m_Profiler.m_IsActive == true )
            m_Profiler.m_Timers[0].SetTime();

    }
    else if( accessMode == "a" || accessMode == "append" )
    {
        if( m_Profiler.m_IsActive == true )
            m_Profiler.m_Timers[0].SetInitialTime();

        m_FileDescriptor = open( m_Name.c_str(),  O_WRONLY | O_APPEND ); //we need to change this

        if( m_Profiler.m_IsActive == true )
            m_Profiler.m_Timers[0].SetTime();
    }
    else if( accessMode == "r" || accessMode == "read" )
    {
        if( m_Profiler.m_IsActive == true )
            m_Profiler.m_Timers[0].SetInitialTime();

        m_FileDescriptor = open( m_Name.c_str(), O_RDONLY );

        if( m_Profiler.m_IsActive == true )
            m_Profiler.m_Timers[0].SetTime();
    }

    if( m_DebugMode == true )
    {
        if( m_FileDescriptor == -1 )
            throw std::ios_base::failure( "ERROR: couldn't open file " + m_Name +
                                          ", from call to Open in FD transport using POSIX open. Does file exists?\n" );
    }
}


void FileDescriptor::Write( const char* buffer, std::size_t size )
{
    if( m_Profiler.m_IsActive == true )
        m_Profiler.m_Timers[1].SetInitialTime();

    auto writtenSize = write( m_FileDescriptor, buffer, size );

    if( m_Profiler.m_IsActive == true )
        m_Profiler.m_Timers[1].SetTime();

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


void FileDescriptor::Close( )
{
    if( m_Profiler.m_IsActive == true )
        m_Profiler.m_Timers[2].SetInitialTime();

    int status = close( m_FileDescriptor );

    if( m_Profiler.m_IsActive == true )
        m_Profiler.m_Timers[2].SetTime();

    if( m_DebugMode == true )
    {
        if( status == -1 )
            throw std::ios_base::failure( "ERROR: couldn't close file " + m_Name +
                                          ", in call to POSIX write\n"   );
    }

    m_IsOpen = false;
}



} //end namespace transport
}//end namespace
