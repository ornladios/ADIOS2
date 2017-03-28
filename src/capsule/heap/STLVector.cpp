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

#include "capsule/heap/STLVector.h"


namespace adios
{
namespace capsule
{


STLVector::STLVector( const std::string accessMode, const int rankMPI, const bool debugMode ):
    Capsule( "Heap", accessMode, rankMPI, debugMode )
{
    m_Data.reserve( 16777216 );
}


STLVector::~STLVector( )
{ }


char* STLVector::GetData( )
{
    return m_Data.data( );
}


char* STLVector::GetMetadata( )
{
    return m_Metadata.data( );
}


std::size_t STLVector::GetDataSize( ) const
{
    return m_Data.size( );
}


std::size_t STLVector::GetMetadataSize( ) const
{
    return m_Metadata.size( );
}


void STLVector::ResizeData( const std::size_t size )
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

void STLVector::ResizeMetadata( const std::size_t size )
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


}  //end namespace heap
}  //end namespace
