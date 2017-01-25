/*
 * ShmSystemV.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: wfg
 */

#include <sys/shm.h>

#include <ios> //std::ios_base::failure

#include "capsule/ShmSystemV.h"
#include "functions/capsuleTemplates.h"


namespace adios
{


ShmSystemV::ShmSystemV( const std::string accessMode, const int rankMPI, const std::string pathName,
                        const size_t dataSize, const size_t metadataSize,
                        const bool debugMode, const unsigned int cores ):
    Capsule( "ShmSystemV", accessMode, rankMPI, debugMode, cores ),
    m_DataSize{ dataSize },
    m_MetadataSize{ metadataSize }
{
    // Data Shared memory sector
    const std::string dataPath( pathName + "/adios.shm.data." + std::to_string( m_RankMPI ) );
    m_DataKey = ftok( dataPath.c_str(), m_RankMPI+1 );
    m_DataShmID = shmget( m_DataKey, m_DataSize, IPC_CREAT | 0666 );
    m_Data = (char*)shmat( m_DataShmID, NULL, 0 );


    // Metadata Shared memory sector
    const std::string metadataPath( pathName + "/adios.shm.metadata." + std::to_string( m_RankMPI ) );
    m_MetadataKey = ftok( metadataPath.c_str(), m_RankMPI+1 ); //2nd field must be greater than zero and unique
    m_MetadataShmID = shmget( m_MetadataKey, m_MetadataSize, IPC_CREAT | 0666 );
    m_Metadata = (char*)shmat( m_MetadataShmID, NULL, 0 );

    if( m_DebugMode == true )
        CheckShm( );
}


ShmSystemV::~ShmSystemV( )
{ }


char* ShmSystemV::GetData( )
{
    return m_Data;
}


char* ShmSystemV::GetMetadata( )
{
    return m_Metadata;
}


std::size_t ShmSystemV::GetDataSize( ) const
{
    return m_DataSize;
}


std::size_t ShmSystemV::GetMetadataSize( ) const
{
    return m_MetadataSize;
}


void ShmSystemV::CheckShm( ) const
{
    if( m_DataShmID < 0)
        throw std::ios_base::failure( "ERROR: Failed to create data shm segment of size " + std::to_string( m_DataSize ) +
                                      " from call to ShmSystemV constructor\n" );

    if( m_Data == nullptr )
        throw std::ios_base::failure( "ERROR: Failed to attach to data shm segment of size " + std::to_string( m_DataSize ) +
                                      " and id " + std::to_string( m_DataShmID ) +
                                      ", from call to ShmSystemV constructor\n" );

    if( m_DataShmID < 0)
        throw std::ios_base::failure( "ERROR: Failed to create metadata shm segment of size " + std::to_string( m_MetadataSize ) +
                                      " from call to ShmSystemV constructor\n" );


    if( m_Metadata == nullptr )
        throw std::ios_base::failure( "ERROR: Failed to attach to metadata shm segment of size " + std::to_string( m_MetadataSize ) +
                                      " and id " + std::to_string( m_MetadataShmID ) +
                                      " from call to ShmSystemV constructor\n" );
}



} //end namespace
