/*
 * WriterTemplate.h
 *
 *  Created on: Jan 12, 2017
 *      Author: wfg
 */

#ifndef WRITERTEMPLATES_H_
#define WRITERTEMPLATES_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
#include <iostream>
#include <memory> //std::shared_ptr
#include <cmath> //std::ceil
#include <new> //std::bad_alloc
/// \endcond


#include "core/Group.h"
#include "core/Variable.h"
#include "capsule/Heap.h"
#include "format/BP1Writer.h"


namespace adios
{

/**
 * Unique template function that replaces macros to write any variable type to a single heap capsule
 * @param group variable owner
 * @param variableName
 * @param variable to be written
 * @param growthFactor buffer growth factor
 * @param maxBufferSize buffer maximum size, if reached transport will be called
 * @param buffers single heap capsule containing data and metadata buffers
 * @param transports all transports from Writer Engine, info is flushed in case of buffer overflow
 * @param bp1Writer from Writer Engine
 */
template <class T>
void WriterWriteVariable( const Group& group, const Var variableName, const Variable<T>& variable,
                          const float growthFactor, const std::size_t maxBufferSize, const int rankMPI,
                          Heap& buffer, std::vector< std::shared_ptr<Transport> >& transports,
                          format::BP1Writer& bp1Writer, const unsigned int cores = 1 )
{
    auto lf_CheckAllocationResult = [ ]( const int result, const std::string variableName, const int rankMPI )
    {
        if( result == -1 )
            throw std::runtime_error( "ERROR: bad_alloc when writing variable " + variableName +
                                      " from rank " + std::to_string( rankMPI ) );
    };

    //Check if data in buffer needs to be reallocated
    const size_t indexSize = bp1Writer.GetVariableIndexSize( group, variableName, variable ); //metadata size
    const std::size_t payloadSize = GetTotalSize( group.GetDimensions( variable.DimensionsCSV ) ) * sizeof( T );
    const std::size_t dataSize = payloadSize + indexSize + 10; //adding some bytes tolerance

    bool doTransportsFlush = false; // might need to write payload in batches
    const std::size_t neededSize = dataSize + buffer.m_DataPosition;
    if( neededSize > maxBufferSize )
    {
        doTransportsFlush = true;
        int result = GrowBuffer( maxBufferSize, growthFactor, buffer.m_DataPosition, buffer.m_Data );
        lf_CheckAllocationResult( result, variableName, rankMPI );
    }
    else
    {
        int result = GrowBuffer( neededSize, growthFactor, buffer.m_DataPosition, buffer.m_Data );
        lf_CheckAllocationResult( result, variableName, rankMPI );
    }

    //WRITE INDEX//
    auto& bpMetadataSet = bp1Writer.m_BPMetadataSets[0];
    int result = GrowBuffer( indexSize, growthFactor, bpMetadataSet.VarsIndexPosition, bpMetadataSet.VarsIndex );
    lf_CheckAllocationResult( result, variableName, rankMPI );

    //Write in BP Format
    std::vector<char*> dataBuffers { buffer.m_Data.data() };
    std::vector<std::size_t> dataPositions { buffer.m_DataPosition }; //needs to be updated
    std::vector<std::size_t> dataAbsolutePositions { buffer.m_DataAbsolutePosition }; //needs to be updated at the end
    std::vector<char*> metadataBuffers { bpMetadataSet.VarsIndex.data() };
    std::vector<std::size_t> metadataPositions { bpMetadataSet.VarsIndexPosition  }; //needs to be updated

    bp1Writer.WriteVariableIndex( group, variableName, variable, dataBuffers, dataPositions,
                                  dataAbsolutePositions, metadataBuffers, metadataPositions );

    buffer.m_DataPosition = dataPositions[0];
    buffer.m_DataAbsolutePosition = dataAbsolutePositions[0];
    bpMetadataSet.VarsIndexPosition = metadataPositions[0];

    //here write payload to data
    if( doTransportsFlush == true ) //in batches
    {

    }
    else
    {
        //this is the expensive part might want to use threaded memcpy
        MemcpyThreads( buffer.m_Data.data(), variable.Values, payloadSize, cores );
        buffer.m_DataPosition += payloadSize;
        buffer.m_DataAbsolutePosition += buffer.m_DataPosition;
    }

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
}





} //end namespace




#endif /* WRITERTEMPLATES_H_ */
