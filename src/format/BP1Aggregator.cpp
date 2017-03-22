/*
 * BP1Aggregator.cpp
 *
 *  Created on: Mar 21, 2017
 *      Author: wfg
 */

#include <vector>
#include <fstream>
#include <iostream> // must be deleted
#include <unistd.h> // must be deleted

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
        unsigned int sizeMPI = static_cast<unsigned int>( m_SizeMPI );
        std::vector< std::vector<char> > rankLogs( sizeMPI - 1 ); //rankLogs from other processes
        std::vector< int > rankLogsSizes( sizeMPI-1, -1 ); //init with -1
        std::vector<MPI_Request> requests( sizeMPI );
        std::vector<MPI_Status> statuses( sizeMPI );

        //first receive sizes
        for( unsigned int i = 1; i < sizeMPI; ++i )
            MPI_Irecv( &rankLogsSizes[i-1], 1, MPI_INT, i, 0, m_MPIComm, &requests[i] );

        for( unsigned int i = 1; i < sizeMPI; ++i )
        {
            MPI_Wait( &requests[i], &statuses[i] );
            if( m_DebugMode == true )
            {
                if( rankLogsSizes[i-1] == -1 )
                    throw std::runtime_error( "ERROR: couldn't get size from rank " + std::to_string(i) + ", in ADIOS aggregator for Profiling.log\n" );
            }
            rankLogs[i-1].resize( rankLogsSizes[i-1] ); //allocate with zeros
        }

        //receive rankLog from other ranks
        for( unsigned int i = 1; i < sizeMPI; ++i )
            MPI_Irecv( rankLogs[i-1].data(), rankLogsSizes[i-1], MPI_CHAR, i, 1, m_MPIComm, &requests[i] );

        for( unsigned int i = 1; i < sizeMPI; ++i )
            MPI_Wait( &requests[i], &statuses[i] );

        //write file
        std::string logFile( "log = { \n" );
        logFile += rankLog + "\n";
        for( unsigned int i = 1; i < sizeMPI; ++i )
        {
            const std::string rankLogStr( rankLogs[i-1].data(), rankLogs[i-1].size() );
            logFile += rankLogStr + "\n";
        }
        logFile += " }\n";

        std::cout << logFile;
        std::ofstream logStream( fileName );
        logStream.write( logFile.c_str(), logFile.size() );
        logStream.close();
    }
    else
    {
        int rankLogSize = static_cast<int>( rankLog.size() );
        MPI_Request requestSize;
        MPI_Isend( &rankLogSize, 1, MPI_INT, 0, 0, m_MPIComm, &requestSize );

        MPI_Request requestRankLog;
        MPI_Isend( const_cast<char*>( rankLog.c_str() ), rankLogSize, MPI_CHAR, 0, 1, m_MPIComm, &requestRankLog );
    }
}



} //end namespace format
} //end namespace adios

