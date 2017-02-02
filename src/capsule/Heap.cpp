/*
 * Heap.cpp
 *
 *  Created on: Dec 22, 2016
 *      Author: wfg
 */

/// \cond EXCLUDE_FROM_DOXYGEN
#include <new> //std::bad_alloc
#include <stdexcept> //std::runtime_error
/// \endcond

#include "capsule/Heap.h"


namespace adios
{


Heap::Heap( const std::string accessMode, const int rankMPI, const bool debugMode ):
    Capsule( "Heap", accessMode, rankMPI, debugMode ),
    m_Data( 16777216, '\0' )
{
    //m_Data.resize( 16777216 ); //default capacity = 16Mb using resize
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
    if( m_DebugMode == true )
    {
        try { m_Data.resize( size ); }
        catch( std::bad_alloc& e ){ throw std::runtime_error( "ERROR: bad_alloc detected when resizing data buffer with size " +
                                                               std::to_string( size ) + "\n" ); }
    }
    else
    {
        m_Data.resize( size );
    }
}

void Heap::ResizeMetadata( const std::size_t size )
{
    if( m_DebugMode == true )
    {
        try { m_Metadata.resize( size ); }
        catch( std::bad_alloc& e ){ throw std::runtime_error( "ERROR: bad_alloc detected when resizing metadata buffer with size " +
                                                               std::to_string( size ) + "\n" ); }
    }
    else
    {
        m_Metadata.resize( size );
    }
}


}  //end namespace
