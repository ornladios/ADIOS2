
#ifndef SHMSYSTEMV_H_
#define SHMSYSTEMV_H_

#include <sys/types.h>
#include <sys/ipc.h>


#include "core/Capsule.h"


namespace adios
{

/**
 * Buffer and Metadata are allocated in virtual memory using interprocess communication (IPC) of Unix's System V
 */
class ShmSystemV : public Capsule
{

public:

    /**
     * Create a Capsule in shared memory using System V shm API
     * @param accessMode
     * @param pathName used to create the key as a unique identifier
     * @param dataSize size of allocated memory segment for data
     * @param metadataSize size of allocated memory segment for metadata
     * @param debugMode true: extra checks, slower
     * @param cores threaded operations
     */
    ShmSystemV( const std::string accessMode, const int rankMPI, const std::string pathName,
                const size_t dataSize, const size_t metadataSize,
                const bool debugMode = false, const unsigned int cores = 1 );

    ~ShmSystemV( );

    char* GetData( ); ///< return the pointer to the raw data buffer
    char* GetMetadata( ); ///< return the pointer to the raw metadata buffer

    const std::size_t GetDataSize( ) const; ///< get current data buffer size
    const std::size_t GetMetadataSize( ) const; ///< get current metadata buffer size

    void WriteData( const std::size_t first, const char* data, const std::size_t size );
    void WriteData( const std::size_t first, const unsigned char* data, const std::size_t size );
    void WriteData( const std::size_t first, const short* data, const std::size_t size );
    void WriteData( const std::size_t first, const unsigned short* data, const std::size_t size );
    void WriteData( const std::size_t first, const int* data, const std::size_t size );
    void WriteData( const std::size_t first, const unsigned int* data, const std::size_t size );
    void WriteData( const std::size_t first, const long int* data, const std::size_t size );
    void WriteData( const std::size_t first, const unsigned long int* data, const std::size_t size );
    void WriteData( const std::size_t first, const long long int* data, const std::size_t size );
    void WriteData( const std::size_t first, const unsigned long long int* data, const std::size_t size );
    void WriteData( const std::size_t first, const float* data, const std::size_t size );
    void WriteData( const std::size_t first, const double* data, const std::size_t size );
    void WriteData( const std::size_t first, const long double* data, const std::size_t size );

    void WriteMetadata( const std::size_t first, const char* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const unsigned char* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const short* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const unsigned short* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const int* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const unsigned int* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const long int* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const unsigned long int* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const long long int* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const unsigned long long int* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const float* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const double* metadata, const std::size_t size );
    void WriteMetadata( const std::size_t first, const long double* metadata, const std::size_t size );


private:

    char* m_Data = nullptr; ///< reference to a shared memory data buffer created with shmget
    const size_t m_DataSize; ///< size of the allocated shared memory segment
    key_t m_DataKey; ///< key associated with the data buffer, created with ftok
    int m_DataShmID; ///< data shared memory buffer id

    char* m_Metadata = nullptr; ///< reference to a shared memory metadata buffer created with shmget
    const size_t m_MetadataSize; ///< size of the allocated shared memory segment
    key_t m_MetadataKey; ///< key associated with the metadata buffer, created with ftok
    int m_MetadataShmID; ///< metadata shared memory buffer id

    void CheckShm( ) const; ///< checks if all shared memory allocations are correct, throws std::bad_alloc, called from constructor if debug mode is true
};

} //end namespace


#endif /* SHMSYSTEMV_H_ */
