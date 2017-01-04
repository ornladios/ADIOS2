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


ShmSystemV::ShmSystemV( const std::string accessMode, const int rankMPI, const std::string pathName,
                        const size_t dataSize, const size_t metadataSize, const unsigned int cores ):
    Capsule( "ShmSystemV", accessMode, rankMPI, cores ),
    m_DataSize{ dataSize },
    m_MetadataSize{ metadataSize }
{
    // Data Shared memory sector
    const std::string dataPath( pathName + "/adios.shm.data." + std::to_string( m_RankMPI ) );
    m_DataKey = ftok( dataPath.c_str(), m_RankMPI+1 );
    m_DataShmID = shmget( m_DataKey, m_DataSize, IPC_CREAT | 0666 );

    if( m_DataShmID < 0)
        throw std::ios_base::failure( "ERROR: Failed to create data shm segment of size " + m_DataSize +
                                      " from call to ShmSystemV constructor\n" );

    m_Data = (char*)shmat( m_DataShmID, NULL, 0 );

    if( m_Data == nullptr )
        throw std::ios_base::failure( "ERROR: Failed to attach to data shm segment of size " + m_DataSize +
                                      " from call to ShmSystemV constructor\n" );

    // Metadata Shared memory sector
    const std::string metadataPath( pathName + "/adios.shm.metadata." + std::to_string( m_RankMPI ) );
    m_MetadataKey = ftok( metadataPath.c_str(), m_RankMPI+1 ); //2nd field must be greater than zero and unique
    m_MetadataShmID = shmget( m_MetadataKey, m_MetadataSize, IPC_CREAT | 0666 );

    if( m_DataShmID < 0)
        throw std::ios_base::failure( "ERROR: Failed to create metadata shm segment of size " + m_MetadataSize +
                                      " from call to ShmSystemV constructor\n" );

    m_Metadata = (char*)shmat( m_MetadataShmID, NULL, 0 );
    if( m_Metadata == nullptr )
        throw std::ios_base::failure( "ERROR: Failed to attach to metadata shm segment of size " + m_MetadataSize +
                                      " from call to ShmSystemV constructor\n" );
}


ShmSystemV::~ShmSystemV( )
{ }


char* ShmSystemV::GetData( ) const
{
    return m_Data;
}


char* ShmSystemV::GetMetadata( ) const
{
    return m_Metadata;
}


const std::size_t ShmSystemV::GetDataSize( ) const
{
    return m_DataSize;
}


const std::size_t ShmSystemV::GetMetadataSize( ) const
{
    return m_MetadataSize;
}


//WriteData functions
void ShmSystemV::WriteData( const std::size_t first, const char* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( char ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const unsigned char* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( unsigned char ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const short* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( short ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const unsigned short* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( unsigned short ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const int* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( int ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const unsigned int* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( unsigned int ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const long int* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( long int ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const unsigned long int* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( unsigned long int ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const long long int* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( long long int ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const unsigned long long int* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( unsigned long long int ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const float* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( float ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const double* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( double ), m_Cores );
}

void ShmSystemV::WriteData( const std::size_t first, const long double* data, const std::size_t size )
{
    MemcpyThreads( &m_Data[first], data, size * sizeof( long double ), m_Cores );
}

//WriteMetadata functions
void ShmSystemV::WriteMetadata( const std::size_t first, const char* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( char ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const unsigned char* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( unsigned char ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const short* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( short ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const unsigned short* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( unsigned short ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const int* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( int ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const unsigned int* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( unsigned int ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const long int* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( long int ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const unsigned long int* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( unsigned long int ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const long long int* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( long long int ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const unsigned long long int* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( unsigned long long int ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const float* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( float ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const double* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( double ), m_Cores );
}

void ShmSystemV::WriteMetadata( const std::size_t first, const long double* metadata, const std::size_t size )
{
    MemcpyThreads( &m_Metadata[first], metadata, size * sizeof( long double ), m_Cores );
}



} //end namespace
