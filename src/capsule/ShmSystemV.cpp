/*
 * ShmSystemV.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: wfg
 */

#include <sys/shm.h>

#include "capsule/ShmSystemV.h"


namespace adios
{


ShmSystemV::ShmSystemV( const std::string accessMode, const int rankMPI, const std::string pathName, const int id,
                        const size_t dataSize, const size_t metadataSize ):
    Capsule( "ShmSystemV", accessMode, rankMPI ),
    m_DataSize{ dataSize },
    m_MetadataSize{ metadataSize }
{
    // Data Shared memory sector
    const std::string dataPath( pathName + "/adios.shm.data." + std::to_string( m_RankMPI ) );
    m_DataKey = ftok( dataPath.c_str(), m_RankMPI+1 );
    m_DataShmID = shmget( m_DataKey, m_DataSize, IPC_CREAT | 0666 );

    // Metadata Shared memory sector
    const std::string metadataPath( pathName + "/adios.shm.metadata." + std::to_string( m_RankMPI ) );
    m_MetadataKey = ftok( metadataPath.c_str(), m_RankMPI+1 );
    m_MetadataShmID = shmget( m_MetadataKey, m_MetadataSize, IPC_CREAT | 0666 );
}


ShmSystemV::~ShmSystemV( )
{ }





}
