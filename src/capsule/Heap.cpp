/*
 * Heap.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: wfg
 */


#include "capsule/Heap.h"
#include "functions/capsuleTemplates.h"


namespace adios
{


Heap::Heap( const std::string accessMode, const int rankMPI, const bool debugMode, const unsigned int cores ):
    Capsule( "Heap", accessMode, rankMPI, debugMode, cores )
{
    m_Data.reserve( 16777216 ); //default capacity = 16Mb
    m_Metadata.reserve( 102400 ); //default capacity = 100Kb
}


Heap::~Heap( )
{ }


char* Heap::GetData( )
{
    return m_Data.data( );
}


char* Heap::GetMetadata( )
{
    return m_Metadata.data( );
}


std::size_t Heap::GetDataSize( ) const
{
    return m_Data.size( );
}


std::size_t Heap::GetMetadataSize( ) const
{
    return m_Metadata.size( );
}


void Heap::ResizeData( const std::size_t size )
{
    m_Data.resize( size );
}


void Heap::ResizeMetadata( const std::size_t size )
{
    m_Metadata.resize( size );
}


//void Heap::Write( const Variable<double>& variable,
//                  const std::vector<unsigned long long int>& localDimensions,
//                  const std::vector<unsigned long long int>& globalDimensions,
//                  const std::vector<unsigned long long int>& globalOffsets,
//                  std::vector< std::shared_ptr<Transport> >& transports )
//{
//    const unsigned long long int localSize = GetTotalSize( localDimensions );
//    double min, max;
//    GetMinMax( variable.Values, localSize, min, max );
//
//    const std::size_t bytesToWrite = localSize * sizeof( double ); //size of values + min + max in bytes
//    const std::size_t currentSize = m_Data.size(); //0 the first time
//    const std::size_t newSize = currentSize + bytesToWrite;
//
//    //current capacity is enough to hold new data
//    if( newSize <= m_Data.capacity() )
//    {
//        m_Data.resize( newSize ); //this resize should not allocate/deallocate
//        MemcpyThreads( &m_Data[currentSize], variable.Values, bytesToWrite, m_Cores );
//        //need to implement metadata write
//        return;
//    }
//
//    //newPosition exceeds current capacity
//    const std::size_t newCapacity = m_GrowthFactor * m_Data.capacity();
//
//    if( newSize <= m_MaxDataSize )
//    {
//        if( newCapacity <= m_MaxDataSize )
//        {
//            m_Data.resize( newCapacity );
//        }
//    }
//
//
//    if( newPosition <= m_MaxDataSize ) // Data is sufficient
//    {
//        m_Data.resize( currentPosition + bytesToWrite );
//        MemcpyThreads( &m_Data[currentPosition], variable.Values, valuesBytes, m_Cores );
//        return;
//    }
//
//
//
//    // dataBytes > maxBufferSize == buffer.size() split the variable in buffer buckets
//    buffer.resize( maxBufferSize ); //resize to maxBufferSize, this might call realloc
//    const size_t buckets =  dataBytes / maxBufferSize + 1;
//    const size_t remainder = dataBytes % maxBufferSize;
//
//    for( unsigned int bucket = 0; buckets < buckets; ++bucket )
//    {
//        const size_t dataOffset = bucket * maxBufferSize / sizeof( T );
//
//        if( bucket == buckets-1 )
//            MemcpyThreads( &buffer[0], data[dataOffset], remainder, 1 );
//        else
//            MemcpyThreads( &buffer[0], data[dataOffset], maxBufferSize, 1 );
//
//        lf_TransportsWrite( transportIndex, transports, buffer );
//    }
//
//
//
//
//}



}  //end namespace
