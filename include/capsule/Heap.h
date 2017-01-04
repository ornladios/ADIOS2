/*
 * Heap.h
 *
 *  Created on: Dec 19, 2016
 *      Author: wfg
 */

#ifndef HEAP_H_
#define HEAP_H_


#include "core/Capsule.h"


namespace adios
{

/**
 * Data and Metadata buffers are allocated in the Heap
 */
class Heap : public Capsule
{

public:

    /**
     * Unique constructor
     * @param accessMode read, write or append
     * @param rankMPI MPI rank
     * @param dataSize maximum data size set by user
     * @param metadataSize maximum metadata size set by user
     */
    Heap( const std::string accessMode, const int rankMPI, const unsigned int cores = 1 );

    ~Heap( );

    char* GetData( ) const;
    char* GetMetadata( ) const;

    const std::size_t GetDataSize( ) const;
    const std::size_t GetMetadataSize( ) const;

    void ResizeData( const std::size_t size );
    void ResizeMetadata( const std::size_t size );

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

    std::vector<char> m_Data; ///< data buffer allocated using the STL in heap memory, default size = 16 Mb
    std::vector<char> m_Metadata; ///< metadata buffer allocated using the STL in heap memory, default size = 100 Kb

    const size_t m_MaxDataSize; ///< maximum data size set by user
    const size_t m_MaxMetadataSize; ///< maximum metadata size set by user
};





} //end namespace






#endif /* HEAP_H_ */
