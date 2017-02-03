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
