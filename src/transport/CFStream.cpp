/*
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <core/CVariable.h>
#include <iostream>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <cstring>
/// \endcond

#include "transport/CFStream.h"
#include "core/CVariable.h"
#include "functions/GroupFunctions.h"
#include "functions/Templates.h"

namespace adios
{


CFStream::CFStream( const unsigned int priority, const unsigned int iteration, MPI_Comm mpiComm, const bool debugMode ):
    CTransport( "CFStream", priority, iteration, mpiComm, debugMode )
{ }


CFStream::~CFStream( )
{ }


void CFStream::Open( const std::string fileName, const std::string accessMode )
{
    m_StreamName = fileName;

    if( m_RankMPI == 0 )
    {
        if( accessMode == "w" || accessMode == "write" )
            m_FStream.open( fileName, std::fstream::out );

        else if( accessMode == "a" || accessMode == "append" )
            m_FStream.open( fileName, std::fstream::out | std::fstream::app );

        else if( accessMode == "r" || accessMode == "read" )
            m_FStream.open( fileName, std::fstream::in );

        if( m_DebugMode == true )
        {
            if( m_FStream.good() == false )
                throw std::ios_base::failure( "ERROR: couldn't open file " + fileName + " in Open function\n" );
        }
    }
    MPI_Barrier( m_MPIComm ); //all of them must wait until the file is opened
}


void CFStream::Write( const CVariable& variable )
{
    //local buffer, to be send over MPI
    std::vector<char> buffer;
    const std::string type( variable.m_Type );
    auto var = GetVariableValues( variable );

//    if( type.find( "vector" )  ) //is a vector
//    {
//        //find total size first
//        //auto values = variable.Get< std::vector<int> >();
//        auto values = GetVariableValues( variable );
//        //auto values = GetVariableValues( variable );
//        unsigned int sizeSum = 0;
//        for( auto element : *values )
//            sizeSum += (int) std::log10( (double) std::abs( element ) ) + 1;
//
//        buffer.reserve( 2*sizeSum );
//
//        for( auto element : *values )
//        {
//            const char* elementChar = std::to_string( element ).c_str();
//            buffer.insert( buffer.end(), elementChar, elementChar + strlen( elementChar ) );
//            buffer.push_back(' ');
//        }
//
//        if( m_RankMPI == 0 )
//        {
//            std::cout << "Writing to file " << m_StreamName << "\n";
//
//            m_FStream << "Hello from rank " << m_RankMPI << " : ";
//            m_FStream.write( &buffer[0], buffer.size() );
//            m_FStream << "\n";
//
//            MPI_Status* status = NULL;
//
//            for( int r = 1; r < m_SizeMPI; ++r )
//            {
//                int bufferSize;
//                MPI_Recv( &bufferSize, 1, MPI_INT, r, 0, m_MPIComm, status ); //receive from r the buffer size
//                std::cout << "Getting from rank: " << r << " buffer size "<< bufferSize << "\n";
//
//                buffer.resize( bufferSize );
//                MPI_Recv( &buffer[0], bufferSize, MPI_CHAR, r, 1, m_MPIComm, status ); //receive from r the buffer
//
//                m_FStream << "Hello from rank " << r << " : ";
//                m_FStream.write( &buffer[0], bufferSize );
//                m_FStream << "\n";
//            }
//        }
//        else
//        {
//            int bufferSize = (int)buffer.size();
//            MPI_Send( &bufferSize, 1, MPI_INT, 0, 0, m_MPIComm ); //send to rank=0 the buffer size
//
//            std::cout << "Hello from rank: " << m_RankMPI << "\n";
//            std::cout << "Buffer size: " << bufferSize << "\n";
//
//            MPI_Send( &buffer[0], bufferSize, MPI_CHAR, 0, 1, m_MPIComm ); //send to rank=0 the buffer
//        }
//
//        MPI_Barrier( m_MPIComm );
//    }
}

void CFStream::Close(  )
{
   if( m_RankMPI == 0 )
       m_FStream.close();

}



} //end namespace

