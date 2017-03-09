/*
 * Transport.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */

#include "core/Transport.h"


namespace adios
{

Transport::Transport( const std::string type, MPI_Comm mpiComm, const bool debugMode ):
    m_Type{ type },
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


Transport::~Transport( )
{ }


void Transport::SetBuffer( char* buffer, size_t size )
{ }


void Transport::Flush( )
{ }


void Transport::Close( )
{ }


void Transport::InitProfiler( const Support::Resolutions resolution )
{
    m_Profiler.m_Timers.emplace_back( "open", Support::Resolutions::mus );

    if( m_AccessMode == "w" || m_AccessMode == "write" )
        m_Profiler.m_Timers.emplace_back( "write", resolution );

    else if( m_AccessMode == "a" || m_AccessMode == "append" )
        m_Profiler.m_Timers.emplace_back( "append", resolution );

    else if( m_AccessMode == "r" || m_AccessMode == "read" )
        m_Profiler.m_Timers.emplace_back( "read", resolution );

    m_Profiler.m_Timers.emplace_back( "close", Support::Resolutions::mus );

    m_Profiler.m_TotalBytes.push_back( 0 );
    m_Profiler.m_IsActive = true;
}



} //end namespace

