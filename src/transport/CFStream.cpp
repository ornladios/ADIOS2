/*
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */

#include <iostream>

#include "transport/CFStream.h"


namespace adios
{


CFStream::CFStream( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm ):
    CTransport( "CFStream", priority, iteration, mpiComm )
{ }


CFStream::~CFStream( )
{ }


void CFStream::Write( const CVariable& variable )
{
    int rank, size;
    MPI_Comm_rank( m_MPIComm, &rank );
    MPI_Comm_size( m_MPIComm, &size );

    std::cout << "Just saying Hello from CFStream Write from process " << rank << "/" << size  << "\n";
    std::cout << "My variable type is " << variable.m_Type << "\n";
}



} //end namespace

