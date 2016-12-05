/*
 * Transport.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */

#include "core/Transport.h"


namespace adios
{


Transport::Transport( const std::string method, MPI_Comm mpiComm, const bool debugMode ):
    m_Method{ method },
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
    MPI_Comm_rank( m_MPIComm, &m_MPIRank );
    MPI_Comm_size( m_MPIComm, &m_MPISize );
}


Transport::~Transport( )
{ }


void Transport::SetBuffer( std::vector<char>& buffer )
{ }


void Transport::Write( std::vector<char>& buffer )
{ }


void Transport::Init( const std::vector<std::string>& arguments )
{ }



} //end namespace

