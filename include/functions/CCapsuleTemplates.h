/*
 * CCapsuleTemplates.h
 *
 *  Created on: Nov 18, 2016
 *      Author: wfg
 */

#ifndef CCAPSULETEMPLATES_H_
#define CCAPSULETEMPLATES_H_


#include <cstring> //std::memcpy

namespace adios
{

/**
 * threaded version of std::memcpy
 * @param dest
 * @param source
 * @param count
 * @param cores
 */
template<class T, class U>
void MemcpyThreads( T* destination, const U* source, std::size_t count, const unsigned int cores = 1 )
{
    if( cores == 1 )
    {
        std::memcpy( &destination[0], &source[0], count );
        return;
    }

    const unsigned long long int stride = (unsigned long long int)std::floor( (unsigned long long int)count/cores );
    const unsigned long long int remainder =  (unsigned long int) count % cores;
    const unsigned long long int last = stride + remainder;

    std::vector<std::thread> memcpyThreads;
    memcpyThreads.reserve( cores );

    for( unsigned int core = 0; core < cores; ++core )
    {
        const size_t initialDestination = stride * core / sizeof(T);
        const size_t initialSource = stride * core / sizeof(U);

        if( core == cores-1 )
            memcpyThreads.push_back( std::thread( std::memcpy, &destination[initialDestination], &source[initialSource], last ) );
        else
            memcpyThreads.push_back( std::thread( std::memcpy, &destination[initialDestination], &source[initialSource], stride ) );
    }

    //Now join the threads
    for( auto& thread : memcpyThreads )
        thread.join( );
}


/**
 * Write data to buffer checking that size of data is no more than maxBufferSize
 * @param data
 * @param size
 * @param buffer
 * @param maxBufferSize
 * @param cores
 */
template<class T>
void WriteToBuffer( const T* data, const size_t size, std::vector<unsigned char>& buffer,
                    const size_t maxBufferSize, CTransport& transport, const unsigned int cores )
{
    const size_t dataBytes = size * sizeof( T );

    //if buffer size is enough send all at once to transport and return
    if( dataBytes <= buffer.size() )
    {
        MemcpyThreads( &buffer[0], data, dataBytes, cores ); //copy memory in threaded fashion, need to test with size
        return;
    }

    if( dataBytes > buffer.size() ) //dataBytes > buffer.size()
    {
        if( dataBytes <= maxBufferSize ) // maxBufferSize > dataBytes > buffer.size()
        {
            buffer.resize( dataBytes );
            MemcpyThreads( &buffer[0], data, dataBytes, cores ); //copy memory in threaded fashion, need to test with size
            return;
        }
        else
        {
            buffer.resize( maxBufferSize ); //resize to maxBufferSize
        }
    }

    // dataBytes > maxBufferSize == buffer.size() split the variable in buffer buckets
    const size_t buckets =  dataBytes / maxBufferSize + 1;
    const size_t remainder = dataBytes % maxBufferSize;


    for( unsigned int bucket = 0; buckets < buckets; ++bucket )
    {
        const size_t dataOffset = bucket * maxBufferSize / sizeof( T );

        if( bucket == buckets-1 )
            MemcpyThreads( &buffer[0], data[dataOffset], remainder, cores );
        else
            MemcpyThreads( &buffer[0], data[dataOffset], maxBufferSize, cores );

        transport.Write( buffer );
    }
}






} //end namespace



#endif /* CCAPSULETEMPLATES_H_ */
