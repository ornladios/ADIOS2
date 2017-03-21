/*
 * BP1Aggregator.cpp
 *
 *  Created on: Mar 21, 2017
 *      Author: wfg
 */

#include <vector>
#include <fstream>

#include "format/BP1Aggregator.h"


namespace adios
{
namespace format
{


BP1Aggregator::BP1Aggregator( MPI_Comm mpiComm, const bool debugMode ):
    m_MPIComm{ mpiComm },
    m_DebugMode{ debugMode }
{
    MPI_Comm_rank( m_MPIComm, &m_RankMPI );
    MPI_Comm_size( m_MPIComm, &m_SizeMPI );
}


BP1Aggregator::~BP1Aggregator( )
{ }



void BP1Aggregator::WriteProfilingLog( const std::string fileName, const std::string& rankLog )
{
    if( m_RankMPI == 0 )
    {
        std::vector< std::vector<char> > rankLogs( m_SizeMPI - 1 ); //rankLogs from other processes

        //first receive sizes
        unsigned int sizeMPI = static_cast<unsigned int>( m_SizeMPI );

        for( unsigned int i = 1; i < sizeMPI; ++i )
        {
            int size = -1;
            MPI_Request request;
            MPI_Irecv( &size, 1, MPI_INT, i, 0, m_MPIComm, &request );

            if( m_DebugMode == true )
            {
                if( size == -1 )
                    throw std::runtime_error( "ERROR: couldn't get size from rank " + std::to_string(i) + ", in ADIOS aggregator for Profiling.log\n" );
                //here check request
            }
            rankLogs[i-1].resize( size ); //allocate with zeros
        }
        //receive rankLog from other ranks
        for( unsigned int i = 1; i < sizeMPI; ++i )
        {
            int size = static_cast<int>( rankLogs[i-1].size() );
            MPI_Request request;
            MPI_Irecv( rankLogs[i-1].data(), size, MPI_CHAR, i, 0, m_MPIComm, &request );

            if( m_DebugMode == true )
            {
                //here check request
            }
        }
        //write file
        std::string logFile( "log = { " + rankLog + "\n" );
        for( unsigned int i = 1; i < sizeMPI; ++i )
        {
            std::string rankLogStr( rankLogs[i-1].data() );
            logFile += rankLogStr + "\n";
        }
        logFile += " }\n";

        std::ofstream logStream( fileName );
        logStream.write( logFile.c_str(), logFile.size() );
        logStream.close();
    }
    else
    {
        int size = static_cast<int>( rankLog.size() );
        MPI_Request request;
        MPI_Isend( &size, 1, MPI_INT, 0, 0, m_MPIComm, &request );
        MPI_Isend( const_cast<char*>( rankLog.c_str() ), size, MPI_CHAR, 0, 0, m_MPIComm, &request );
    }
}



} //end namespace format
} //end namespace adios

