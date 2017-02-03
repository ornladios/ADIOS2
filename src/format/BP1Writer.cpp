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


BP1Writer::BP1Writer( const unsigned int metadataSets ):
    m_BPMetadataSets( metadataSets )
{ }


BP1Writer::~BP1Writer( )
{ }



void BP1Writer::Close( const BP1MetadataSet& metadataSet, Capsule& capsule, Transport& transport )
{



}

//PRIVATE FUNCTIONS
void BP1Writer::WriteNameRecord( const std::string& name, const std::uint16_t& length,
                                 std::vector<char*>& buffers, std::vector<std::size_t>& positions )
{
    MemcpyToBuffers( buffers, positions, &length, 2 );
    MemcpyToBuffers( buffers, positions, name.c_str( ), length );
}


void BP1Writer::WriteDimensionRecord( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                                      const std::vector<unsigned long long int>& localDimensions,
                                      const std::vector<unsigned long long int>& globalDimensions,
                                      const std::vector<unsigned long long int>& globalOffsets,
                                      const bool addType )
{
    if( addType == true )
    {
        constexpr char no = 'n'; //dimension format unsigned int value for now
        for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
        {
            MemcpyToBuffers( buffers, positions, &no, 1 );
            MemcpyToBuffers( buffers, positions, &localDimensions[d], 8 );
            MemcpyToBuffers( buffers, positions, &no, 1 );
            MemcpyToBuffers( buffers, positions, &globalDimensions[d], 8 );
            MemcpyToBuffers( buffers, positions, &no, 1 );
            MemcpyToBuffers( buffers, positions, &globalOffsets[d], 8 );
        }
    }
    else
    {
        for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
        {
            MemcpyToBuffers( buffers, positions, &localDimensions[d], 8 );
            MemcpyToBuffers( buffers, positions, &globalDimensions[d], 8 );
            MemcpyToBuffers( buffers, positions, &globalOffsets[d], 8 );
        }
    }
}

void BP1Writer::WriteDimensionRecord( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                                      const std::vector<unsigned long long int>& localDimensions,
                                      const unsigned int skip,
                                      const bool addType )
{

    if( addType == true )
    {
        constexpr char no = 'n'; //dimension format unsigned int value (not using memberID for now)
        for( const auto& localDimension : localDimensions )
        {
            MemcpyToBuffers( buffers, positions, &no, 1 );
            MemcpyToBuffers( buffers, positions, &localDimension, 8 );
            MovePositions( skip, positions );
        }
    }
    else
    {
        for( const auto& localDimension : localDimensions )
        {
            MemcpyToBuffers( buffers, positions, &localDimension, 8 );
            MovePositions( skip, positions );
        }
    }
}


void BP1Writer::ClosePOSIX( Capsule& capsule, Transport& transport )
{

//
//    const char* dataBuffer = capsule.GetData( );
//    //write data buffer first up to data position
//    transport.Write( dataBuffer, capsule.m_DataPosition );
}




void BP1Writer::SetMiniFooter( BP1MetadataSet& metadataSet )
{



}


void SetMetadata( const BP1MetadataSet& metadataSet, Capsule& capsule )
{

    //setup metadata to capsule metadata buffer
    //    const std::size_t processGroupIndexSize = m_ProcessGroupsLength + 16;
    //    const std::size_t variableIndexSize = m_VariablesLength + 12;
    //    const std::size_t attributeIndexSize = m_AttributesLength + 12;
    //    const std::size_t minifooterSize = 28; //28
    //    const std::size_t metadataSize = processGroupIndexSize + variableIndexSize + attributeIndexSize + minifooterSize;
    //
    //    capsule.ResizeMetadata( metadataSize );
}




} //end namespace format
} //end namespace adios
