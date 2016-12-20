/*
 * CFStream.cpp
 *
 *  Created on: Oct 24, 2016
 *      Author: wfg
 */

/// \cond EXCLUDED_FROM_DOXYGEN
#include <iostream>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <cstring>
/// \endcond

#include "functions/adiosFunctions.h" //CreateDirectory
#include "transport/FStream.h"


namespace adios
{


FStream::FStream( MPI_Comm mpiComm, const bool debugMode, const std::vector<std::string>& arguments ):
    Transport( "FStream", mpiComm, debugMode )
{
    //here do something with arguments
}


FStream::~FStream( )
{ }


void FStream::Open( const std::string streamName, const std::string accessMode )
{
    const std::string directory( streamName + ".dir" ); //data.bp.dir

    if( m_MPIRank == 0 )
        CreateDirectory( directory );

    MPI_Barrier( m_MPIComm ); //all processor wait until directory is created

    const std::string streamNameRank( directory + "/" + streamName + "." + std::to_string( m_MPIRank ) ); //data.bp.dir./data.bp.Rank

    if( accessMode == "w" || accessMode == "write" )
        m_FStream.open( streamNameRank, std::fstream::out );

    else if( accessMode == "a" || accessMode == "append" )
        m_FStream.open( streamNameRank, std::fstream::out | std::fstream::app );

    else if( accessMode == "r" || accessMode == "read" )
        m_FStream.open( streamNameRank, std::fstream::in );

    if( m_DebugMode == true )
    {
        if( !m_FStream )
            throw std::ios_base::failure( "ERROR: couldn't open file " + streamName + " in Open function of FStream transport\n" );
    }

    MPI_Barrier( m_MPIComm ); //all of them must wait until the file is opened
}


void FStream::SetBuffer( std::vector<char>& buffer )
{
    //m_FStream.rdbuf()->pubsetbuf( &buffer[0], buffer.size() );
}

void FStream::Write( const Capsule& capsule )
{
    //m_FStream.write( &buffer[0], buffer.size() );
}


void FStream::Close( const Capsule& capsule )
{
    m_FStream.close();
}

//void CFStream::Write( const CVariable& variable ) ///this is aggregation
//{
//    //local buffer, to be send over MPI
//    std::vector<char> buffer;
//    const std::string type( variable.m_Type );
//    auto var = GetVariableValues( variable );

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
//}


} //end namespace

