/*
 * BP1Writer.cpp
 *
 *  Created on: Feb 1, 2017
 *      Author: wfg
 */


#include "format/BP1Writer.h"



namespace adios
{
namespace format
{


void BP1Writer::CheckVariableIndexSize( const std::size_t newIndexSize )
{
    const std::size_t currentCapacity = m_VariableIndex.capacity( );
    const std::size_t availableSpace =  currentCapacity - m_VariableIndexPosition;

    if( newIndexSize > availableSpace )
        GrowBuffer( newIndexSize, m_VariableIndexPosition, m_VariableIndex );
}


void BP1Writer::GrowBuffer( const std::size_t newDataSize, const std::size_t currentPosition, std::vector<char>& buffer )
{
    const size_t currentCapacity = buffer.capacity( );
    std::size_t newCapacity = static_cast<std::size_t>( std::ceil( m_GrowthFactor * currentCapacity ) );
    std::size_t newAvailableSpace = newCapacity - currentPosition;

    while( newDataSize > newAvailableSpace )
    {
        newCapacity = static_cast<std::size_t>( std::ceil( m_GrowthFactor * newCapacity ) );
        newAvailableSpace = newCapacity - currentPosition;
    }

    m_VariableIndex.resize( newCapacity );
}



} //end namespace format
} //end namespace adios
