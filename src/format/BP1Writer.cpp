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


std::size_t BP1Writer::GetProcessGroupIndexSize( const std::string name, const std::string timeStepName,
                                                 const size_t numberOfTransports ) const noexcept
{
    //pgIndex + list of methods (transports)
    return ( name.length() + timeStepName.length() + 23 ) + ( 3 + numberOfTransports ); //should be sufficient for data and metadata pgindices
}


void BP1Writer::WriteProcessGroupIndex( const bool isFortran, const std::string name, const unsigned int processID,
                                        const std::string timeStepName, const unsigned int timeStep,
                                        const std::vector< std::shared_ptr<Transport> >& transports,
                                        capsule::STLVector& buffer, BP1MetadataSet& metadataSet ) const noexcept
{
    // adapt this part to local variables
    std::vector<char*> dataBuffers{ buffer.m_Data.data() };
    std::vector<size_t> dataPositions{ buffer.m_DataPosition };
    std::vector<size_t> dataAbsolutePositions{ buffer.m_DataAbsolutePosition };

    std::vector<char*> metadataBuffers{ metadataSet.PGIndex.data() };
    std::vector<std::size_t> metadataPositions{ metadataSet.PGIndexPosition };

    const std::vector<int> methodIDs = GetMethodIDs( transports );

    WriteProcessGroupIndexCommon( isFortran, name, processID, timeStepName, timeStep, methodIDs,
                                  dataBuffers, dataPositions, dataAbsolutePositions,
                                  metadataBuffers, metadataPositions );

    buffer.m_DataPosition = dataPositions[0];
    buffer.m_DataAbsolutePosition = dataAbsolutePositions[0];
    metadataSet.PGIndexPosition = metadataPositions[0];

    ++metadataSet.PGCount;
}


void BP1Writer::WriteProcessGroupIndex( const bool isFortran, const std::string name, const unsigned int processID,
                                        const std::string timeStepName, const unsigned int timeStep,
                                        const std::vector< std::shared_ptr<Transport> >& transports,
                                        std::vector< std::shared_ptr<Capsule> >& capsules,
                                        std::vector<BP1MetadataSet>& metadataSets ) const noexcept
{

    // adapt this part to local variables
    std::vector<char*> metadataBuffers, dataBuffers;
    std::vector<std::size_t> metadataPositions, dataPositions, dataAbsolutePositions;

    for( auto& metadataSet : metadataSets )
    {
        metadataBuffers.push_back( metadataSet.PGIndex.data() );
        metadataPositions.push_back( metadataSet.PGIndexPosition );
    }

    for( auto& capsule : capsules )
    {
        dataBuffers.push_back( capsule->GetData( ) );
        dataPositions.push_back( capsule->m_DataPosition );
        dataAbsolutePositions.push_back( capsule->m_DataAbsolutePosition );
    }

    const std::vector<int> methodIDs = GetMethodIDs( transports );

    WriteProcessGroupIndexCommon( isFortran, name, processID, timeStepName, timeStep, methodIDs,
                                  dataBuffers, dataPositions, dataAbsolutePositions,
                                  metadataBuffers, metadataPositions );

    //update positions and varsCount originally passed by value
    const unsigned int buffersSize = static_cast<unsigned int>( capsules.size() );
    for( unsigned int i = 0; i < buffersSize; ++i )
    {
        metadataSets[i].PGIndexPosition = metadataPositions[i];

        capsules[i]->m_DataPosition = dataPositions[i];
        capsules[i]->m_DataAbsolutePosition = dataAbsolutePositions[i];
    }
}


void BP1Writer::Close( BP1MetadataSet& metadataSet, Capsule& capsule,
                       Transport& transport, bool& isFirstClose ) const noexcept
{
    if( isFirstClose == true )
    {
        FlattenMetadata( metadataSet, capsule ); //now capsule
        isFirstClose = false;
    }
    //implementing N-to-N for now, no aggregation
    transport.Write( capsule.GetData(), capsule.m_DataPosition );
    transport.Write( capsule.GetMetadata(), capsule.GetMetadataSize() ); //we can improve this by copying metadata to data
    transport.Close();
}


void BP1Writer::WriteProcessGroupIndexCommon( const bool isFortran, const std::string name, const unsigned int processID,
                                              const std::string timeStepName, const unsigned int timeStep,
                                              const std::vector<int>& methodIDs,
                                              std::vector<char*>& dataBuffers, std::vector<std::size_t>& dataPositions,
                                              std::vector<std::size_t>& dataAbsolutePositions,
                                              std::vector<char*>& metadataBuffers,
                                              std::vector<std::size_t>& metadataPositions ) const noexcept
{
    std::vector<std::size_t> metadataLengthPositions( metadataPositions ); //get length of pg position

    MovePositions( 2, metadataPositions ); //skip length of pg in metadata, 2 bytes, would write at the end
    MovePositions( 8, dataPositions ); //skip length of pg including data, 8 bytes, would write at the end

    //write name to metadata
    const std::uint16_t lengthOfName = name.length();
    WriteNameRecord( name, lengthOfName, metadataBuffers, metadataPositions );

    //write is host language Fortran in metadata and data
    const char hostFortran = ( isFortran ) ? 'y' : 'n'; //if host language is fortran
    MemcpyToBuffers( metadataBuffers, metadataPositions, &hostFortran, 1 );
    MemcpyToBuffers( dataBuffers, dataPositions, &hostFortran, 1 );

    //name in data
    WriteNameRecord( name, lengthOfName, dataBuffers, dataPositions );

    //processID,
    MemcpyToBuffers( metadataBuffers, metadataPositions, &processID, 4 );
    //skip coordination var in data ....what is coordination var?
    MovePositions( 4, dataPositions );

    //time step name to metadata and data
    const std::uint16_t lengthOfTimeStep = timeStepName.length();
    WriteNameRecord( timeStepName, lengthOfTimeStep, metadataBuffers, metadataPositions );
    WriteNameRecord( timeStepName, lengthOfTimeStep, dataBuffers, dataPositions );
    //time step to metadata and data
    MemcpyToBuffers( metadataBuffers, metadataPositions, &timeStep, 4 );
    MemcpyToBuffers( dataBuffers, dataPositions, &timeStep, 4 );

    //write offset to pg in data on metadata which is the current absolute position
    MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 );

    //get pg index length
    std::vector<std::uint16_t> metadataIndexLengths( metadataPositions.size() );
    for( unsigned int i = 0; i < metadataPositions.size(); ++i )
        metadataIndexLengths[i] = metadataPositions[i] - metadataLengthPositions[i];

    //write to metadata length position the pgIndex length
    MemcpyToBuffers( metadataBuffers, metadataLengthPositions, metadataIndexLengths, 2 );
    MovePositions( -2, metadataLengthPositions ); //back to original position
}

//PRIVATE FUNCTIONS
void BP1Writer::WriteNameRecord( const std::string name, const std::uint16_t length,
                                 std::vector<char*>& buffers, std::vector<std::size_t>& positions ) const noexcept
{
    MemcpyToBuffers( buffers, positions, &length, 2 );
    MemcpyToBuffers( buffers, positions, name.c_str( ), length );
}


void BP1Writer::WriteDimensionRecord( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                                      const std::vector<std::size_t>& localDimensions,
                                      const std::vector<std::size_t>& globalDimensions,
                                      const std::vector<std::size_t>& globalOffsets,
                                      const bool addType ) const noexcept
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
                                      const std::vector<std::size_t>& localDimensions,
                                      const unsigned int skip,
                                      const bool addType ) const noexcept
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



void BP1Writer::FlattenMetadata( BP1MetadataSet& metadataSet, Capsule& capsule ) const noexcept
{
    //Finish writing metadata counts and lengths (IndexPosition)
    const std::size_t pgLength = metadataSet.PGIndexPosition;
    std::memcpy( &metadataSet.PGIndex[0], &metadataSet.PGCount, 8 );
    std::memcpy( &metadataSet.PGIndex[8], &pgLength, 8 );

    const std::size_t varsIndexLength = metadataSet.VarsIndexPosition;
    std::memcpy( &metadataSet.VarsIndex[0], &metadataSet.VarsCount, 4 );
    std::memcpy( &metadataSet.VarsIndex[4], &varsIndexLength, 8 );

    const std::size_t attributesIndexLength = metadataSet.AttributesIndexPosition;
    std::memcpy( &metadataSet.AttributesIndex[0], &metadataSet.AttributesCount, 4 );
    std::memcpy( &metadataSet.AttributesIndex[4], &attributesIndexLength, 8 );

    const std::size_t metadataSize = pgLength + varsIndexLength + attributesIndexLength + metadataSet.MiniFooter.size();

    capsule.ResizeMetadata( metadataSize );
    char* metadata = capsule.GetMetadata();

    std::memcpy( &metadata[0], metadataSet.PGIndex.data(), pgLength );
    std::memcpy( &metadata[pgLength], metadataSet.VarsIndex.data(), varsIndexLength );
    std::memcpy( &metadata[varsIndexLength], metadataSet.AttributesIndex.data(), attributesIndexLength );

    //getting absolute offsets, minifooter is 28 bytes for now
    const std::uint64_t offsetPGIndex = capsule.m_DataAbsolutePosition;
    const std::uint64_t offsetVarsIndex = offsetPGIndex + pgLength;
    const std::uint64_t offsetAttributeIndex = offsetVarsIndex + varsIndexLength;
    std::size_t position = pgLength + varsIndexLength + attributesIndexLength;

    //offsets
    std::memcpy( &metadata[position], &offsetPGIndex, 8 );
    std::memcpy( &metadata[position+8], &offsetVarsIndex, 8 );
    std::memcpy( &metadata[position+16], &offsetAttributeIndex, 8 );

    position += 24; //position position to version record
    if( IsLittleEndian() == true )//little endian machine
    {
        constexpr std::uint8_t littleEndian = 0;
        std::memcpy( &metadata[position], &littleEndian, 1 );
    }
    else //big endian
    {
        constexpr std::uint8_t bigEndian = 1;
        std::memcpy( &metadata[position], &bigEndian, 1 );
    }
    position += 3;
    std::memcpy( &metadata[position], &m_Version, 1 );
}





} //end namespace format
} //end namespace adios
