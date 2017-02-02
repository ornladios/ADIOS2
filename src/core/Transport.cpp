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
    MPI_Comm_rank( m_MPIComm, &m_MPIRank );
    MPI_Comm_size( m_MPIComm, &m_MPISize );
}


Transport::~Transport( )
{ }


void Transport::SetBuffer( char* buffer, size_t size )
{ }


void Transport::Flush( )
{ }


void Transport::Close( )
{ }



} //end namespace

