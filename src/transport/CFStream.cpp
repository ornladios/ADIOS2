/*
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */


#include <iostream>
#include <sstream>
#include <cmath>

#include "transport/CFStream.h"


namespace adios
{


CFStream::CFStream( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm ):
    CTransport( "CFStream", priority, iteration, mpiComm )
{ }


CFStream::~CFStream( )
{ }


void CFStream::Open( const std::string fileName, const std::string accessMode )
{
    if( accessMode == "w" || accessMode == "write" )
    {
        int rank, size;
        MPI_Comm_rank( m_MPIComm, &rank );
        MPI_Comm_size( m_MPIComm, &size ); //would write to file

        if( rank == 0 )
        {
            m_FStream.open( fileName, std::fstream::out );
            MPI_Barrier( m_MPIComm );
        }
        else
        {
            MPI_Barrier( m_MPIComm );
            m_FStream.open( fileName, std::fstream::out | std::fstream::app );
        }
    }
    else if( accessMode == "a" || accessMode == "append" )
        m_FStream.open( fileName, std::fstream::out | std::fstream::app );

    else if( accessMode == "r" || accessMode == "read" )
        m_FStream.open( fileName, std::fstream::in );
}

void CFStream::Write( const CVariableBase& variable )
{
    //This part should go to a capsule ??
    int rank, size;
    MPI_Comm_rank( m_MPIComm, &rank );
    MPI_Comm_size( m_MPIComm, &size ); //would write to file

    //local buffer, to be send over MPI
    std::vector<char> buffer;

    if( variable.m_Type.find( "vector" )  ) //is a vector
    {
        //find total size first
        auto values = variable.Get< std::vector<int> >();
        unsigned int sizeSum = 0;
        for( auto element : *values )
            sizeSum += (int) std::log10( (double) std::abs( element ) ) + 1;

        buffer.reserve( sizeSum );

        for( auto element : *values )
        {
            const char* elementChar = std::to_string( element ).c_str();
            buffer.insert( buffer.end(), elementChar, elementChar + sizeof( elementChar ) + 1 );
        }

        if( rank == 0 )
        {
            m_FStream.write( &buffer[0], buffer.size() );
            MPI_Status* status = NULL;

            for( int r = 1; r < size; ++r )
            {
                int bufferSize;
                MPI_Recv( &bufferSize, 1, MPI_INT, r, 0, m_MPIComm, status );

                buffer.resize( bufferSize );
                MPI_Recv( &buffer[0], 1, MPI_CHAR, r, 1, m_MPIComm, status );
                m_FStream.write( &buffer[0], buffer.size() );
            }
        }
        else
        {
            int bufferSize = buffer.size();
            MPI_Send( &bufferSize, 1, MPI_INT, rank, 0, m_MPIComm );
            MPI_Send( &buffer[0], buffer.size(), MPI_CHAR, rank, 1, m_MPIComm );
        }

        MPI_Barrier( m_MPIComm );
    }
}

void CFStream::Close(  )
{
   m_FStream.close();
}



} //end namespace

