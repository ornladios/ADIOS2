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
    metadataSet.DataPGLengthPosition = buffer.m_DataPosition;

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

    metadataSet.DataVarsCountPosition = dataPositions[0];
    metadataSet.PGCount += 1;

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

    unsigned int index = 0;
    for( auto& capsule : capsules )
    {
        dataBuffers.push_back( capsule->GetData( ) );
        dataPositions.push_back( capsule->m_DataPosition );
        dataAbsolutePositions.push_back( capsule->m_DataAbsolutePosition );

        metadataSets[index].DataPGLengthPosition = capsule->m_DataPosition;
        ++index;
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
        metadataSets[i].DataVarsCountPosition = dataPositions[i];

        metadataSets[i].PGCount += 1;
    }
}


void BP1Writer::Close( BP1MetadataSet& metadataSet, Capsule& capsule, Transport& transport, bool& isFirstClose,
    		           const bool haveMetadata, const bool haveTiming ) const noexcept
{
    if( isFirstClose == true )
    {
        FlattenData( metadataSet, capsule );
        FlattenMetadata( metadataSet, capsule );
        isFirstClose = false;
    }
    //implementing N-to-N for now, no aggregation
    transport.Write( capsule.GetData(), capsule.m_DataPosition );
    transport.Write( capsule.GetMetadata(), capsule.m_MetadataPosition ); //we can improve this by copying metadata to data

    if( haveMetadata == true )
    {
        //here call aggregator
    }

    transport.Close();
}



//PRIVATE FUNCTIONS
void BP1Writer::WriteProcessGroupIndexCommon( const bool isFortran, const std::string name, const unsigned int processID,
                                              const std::string timeStepName, const unsigned int timeStep,
                                              const std::vector<int>& methodIDs,
                                              std::vector<char*>& dataBuffers, std::vector<std::size_t>& dataPositions,
                                              std::vector<std::size_t>& dataAbsolutePositions,
                                              std::vector<char*>& metadataBuffers,
                                              std::vector<std::size_t>& metadataPositions ) const noexcept
{
	const std::vector<std::size_t> pgLengthDataPositions( dataPositions );
    std::vector<std::size_t> pgLengthPositions( metadataPositions ); //get length of pg position

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
        metadataIndexLengths[i] = metadataPositions[i] - pgLengthPositions[i] - 2; //without length of group record?

    //write to metadata length position the pgIndex length
    MemcpyToBuffers( metadataBuffers, pgLengthPositions, metadataIndexLengths, 2 );

    //here write method
    const std::uint8_t methodsSize = methodIDs.size();
    MemcpyToBuffers( dataBuffers, dataPositions, &methodsSize, 1 );
    MemcpyToBuffers( dataBuffers, dataPositions, &methodsSize, 2 ); //assume one byte for methodID for now

    for( auto& methodID : methodIDs )
    	MemcpyToBuffers( dataBuffers, dataPositions, &methodID, 1 ); //method ID

    //dataAbsolutePositions need to be updated
    for( unsigned int i = 0; i < dataPositions.size(); ++i )
        dataAbsolutePositions[i] += dataPositions[i] - pgLengthDataPositions[i];
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


void BP1Writer::FlattenData( BP1MetadataSet& metadataSet, Capsule& capsule ) const noexcept
{
    //Finish writing pg group length and, vars count and length in Data
    char* data = capsule.GetData();
    const std::uint64_t dataPGLength = capsule.m_DataPosition - metadataSet.DataPGLengthPosition - 8 + 12; //without record itself, 12 due to empty attributes
    std::memcpy( &data[metadataSet.DataPGLengthPosition], &dataPGLength, 8 );

    //vars count
    std::memcpy( &data[metadataSet.DataVarsCountPosition], &metadataSet.VarsCount, 4 );

    //vars length
    const std::uint64_t dataVarsLength = capsule.m_DataPosition - metadataSet.DataVarsCountPosition - 8 - 4; //without record itself
    std::memcpy( &data[metadataSet.DataVarsCountPosition+4], &dataVarsLength, 8 );

    //here add empty attributes
    capsule.m_DataPosition += 12;
    capsule.m_DataAbsolutePosition += 12;
}


void BP1Writer::FlattenMetadata( BP1MetadataSet& metadataSet, Capsule& capsule ) const noexcept
{
	//Finish writing metadata counts and lengths (IndexPosition)
    const std::uint64_t pgIndexCount = metadataSet.PGCount;
    const std::uint64_t pgIndexLength = metadataSet.PGIndexPosition - 16; //without record itself
    std::size_t pgIndexPosition = 0;
    MemcpyToBuffer( metadataSet.PGIndex.data(), pgIndexPosition, &pgIndexCount, 8 );
    MemcpyToBuffer( metadataSet.PGIndex.data(), pgIndexPosition, &pgIndexLength, 8 );

    const std::uint32_t varsIndexCount = metadataSet.VarsCount;
    const std::uint64_t varsIndexLength = metadataSet.VarsIndexPosition - 12; //without record itself
    std::size_t varsIndexPosition = 0;
    MemcpyToBuffer( metadataSet.VarsIndex.data(), varsIndexPosition, &varsIndexCount, 4 );
    MemcpyToBuffer( metadataSet.VarsIndex.data(), varsIndexPosition, &varsIndexLength, 8 );

    const std::uint32_t attributesIndexCount = metadataSet.AttributesCount;
    const std::uint64_t attributesIndexLength = metadataSet.AttributesIndexPosition - 12; //without record itself
    std::size_t attributesIndexPosition = 0;
    MemcpyToBuffer( metadataSet.AttributesIndex.data(), attributesIndexPosition, &attributesIndexCount, 4 );
    MemcpyToBuffer( metadataSet.AttributesIndex.data(), attributesIndexPosition, &attributesIndexLength, 8 );

    const std::size_t metadataSize = pgIndexLength + varsIndexLength + attributesIndexLength + metadataSet.MiniFooterSize + 40;
    capsule.ResizeMetadata( metadataSize );
    char* metadata = capsule.GetMetadata();

    std::size_t position = 0;
    MemcpyToBuffer( metadata, position, metadataSet.PGIndex.data(), metadataSet.PGIndexPosition );
    MemcpyToBuffer( metadata, position, metadataSet.VarsIndex.data(), metadataSet.VarsIndexPosition );
    MemcpyToBuffer( metadata, position, metadataSet.AttributesIndex.data(), metadataSet.AttributesIndexPosition );

    //getting absolute offsets, minifooter is 28 bytes for now
    const std::uint64_t offsetPGIndex = capsule.m_DataAbsolutePosition;
    MemcpyToBuffer( metadata, position, &offsetPGIndex, 8 );

    const std::uint64_t offsetVarsIndex = offsetPGIndex + metadataSet.PGIndexPosition;
    MemcpyToBuffer( metadata, position, &offsetVarsIndex, 8 );

    const std::uint64_t offsetAttributeIndex = offsetVarsIndex + metadataSet.VarsIndexPosition;
    MemcpyToBuffer( metadata, position, &offsetAttributeIndex, 8 );

    //version
    if( IsLittleEndian( ) )
    {
    	const std::uint8_t endian = 0;
    	MemcpyToBuffer( metadata, position, &endian, 1 );
    	position += 2;
    	MemcpyToBuffer( metadata, position, &m_Version, 1 );
    }
    else
    {

    }

    capsule.m_MetadataPosition = position;
}





} //end namespace format
} //end namespace adios
