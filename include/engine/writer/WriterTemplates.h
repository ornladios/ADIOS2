/*
 * WriterTemplate.h
 *
 *  Created on: Jan 12, 2017
 *      Author: wfg
 */

#ifndef WRITERTEMPLATES_H_
#define WRITERTEMPLATES_H_

#include <string>
#include <vector>
#include <iostream>
#include <memory>


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
 * @param variable
 * @param buffers single heap capsule containing data and metadata buffers
 * @param transports all transports from Writer Engine, info is flushed in case of buffer overflow
 * @param bp1Writer from Writer Engine
 */
template <class T>
void WriterWriteVariable( const Group& group, const std::string variableName, const Variable<T>& variable,
                          Heap& buffer, std::vector< std::shared_ptr<Transport> >& transports,
                          format::BP1Writer& bp1Writer )
{

    std::cout << "Hello from bp Writer, writing variable " << variableName << " of typeid(T).name() = " << typeid(T).name() << "\n";
    if( variable.IsDimension )
    {
        std::cout << "Which is a dimension variable\n";
    }


    //here deal with buffers allocation


//    const auto localDimensions = group.GetDimensions( variable.DimensionsCSV );
//    const auto size = GetTotalSize( localDimensions );
//    T min, max;
//    GetMinMax( variable.Values, size, min, max );

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
