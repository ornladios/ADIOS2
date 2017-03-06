/*
 * BP1.h
 *
 *  Created on: Jan 24, 2017
 *      Author: wfg
 */

#ifndef BP1WRITER_H_
#define BP1WRITER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <vector>
#include <cstdint>  //std::intX_t fixed size integers
#include <algorithm> //std::count, std::copy, std::for_each
#include <cstring> //std::memcpy
#include <cmath>   //std::ceil
/// \endcond

#include "BP1.h"
#include "core/Variable.h"
#include "core/Capsule.h"
#include "capsule/heap/STLVector.h"
#include "functions/adiosTemplates.h"
#include "functions/adiosFunctions.h"


namespace adios
{
namespace format
{


class BP1Writer : public BP1
{

public:

    unsigned int m_Cores = 1;  ///< number of cores for thread operations in large array (min,max)
    unsigned int m_Verbosity = 0; ///< statistics verbosity, can change if redefined in Engine method.
    float m_GrowthFactor = 1.5; ///< memory growth factor, can change if redefined in Engine method.
    const std::uint8_t m_Version = 3; ///< BP format version

    /**
     * Calculates the Process Index size in bytes according to the BP format, including list of method with no parameters (for now)
     * @param name
     * @param timeStepName
     * @param numberOfTransports
     * @return size of pg index
     */
    std::size_t GetProcessGroupIndexSize( const std::string name, const std::string timeStepName,
                                          const std::size_t numberOfTransports ) const noexcept;

    /**
     * Writes a process group index PGIndex and list of methods (from transports), done at Open or aggregation of new time step
     * Version that operates on a single heap buffer and metadataset.
     * @param isFortran
     * @param name
     * @param processID
     * @param timeStepName
     * @param timeStep
     * @param transports
     * @param buffer
     * @param metadataSet
     */
    void WriteProcessGroupIndex( const bool isFortran, const std::string name, const unsigned int processID,
                                 const std::string timeStepName, const unsigned int timeStep,
                                 const std::vector< std::shared_ptr<Transport> >& transports,
                                 capsule::STLVector& buffer,
                                 BP1MetadataSet& metadataSet ) const noexcept;
    /**
     * Writes a process group index PGIndex and list of methods (from transports), done at Open or aggregation of new time step
     * Version that operates on many capsules and metadatasets
     * @param isFortran
     * @param name
     * @param processID
     * @param timeStepName
     * @param timeStep
     * @param transports
     * @param capsules
     * @param metadataSets
     */
    void WriteProcessGroupIndex( const bool isFortran, const std::string name, const unsigned int processID,
                                 const std::string timeStepName, const unsigned int timeStep,
                                 const std::vector< std::shared_ptr<Transport> >& transports,
                                 std::vector< std::shared_ptr<Capsule> >& capsules,
                                 std::vector<BP1MetadataSet>& metadataSets ) const noexcept;

    /**
     * Returns the estimated variable index size
     * @param group
     * @param variableName
     * @param variable
     * @param verbosity
     * @return variable index size
     */
    template< class T >
    size_t GetVariableIndexSize( const Variable<T>& variable ) const noexcept
    {
        //size_t indexSize = varEntryLength + memberID + lengthGroupName + groupName + lengthVariableName + lengthOfPath + path + datatype
        size_t indexSize = 23; //without characteristics
        indexSize += variable.m_Name.size();

        // characteristics 3 and 4, check variable number of dimensions
        const std::size_t dimensions = variable.DimensionsSize(); //number of commas in CSV + 1
        indexSize += 28 * dimensions; //28 bytes per dimension
        indexSize += 1; //id

        //characteristics 3, variable offset in data
        indexSize += 8;
        indexSize += 1; //id
        //characteristics 6, variable payload offset in data
        indexSize += 8;
        indexSize += 1; //id

        //characteristic 0, if scalar add value, for now only allowing string
        if( dimensions == 1 )
        {
            indexSize += sizeof(T);
            indexSize += 1; //id
            //must have an if here
            indexSize += 2 + variable.m_Name.size();
            indexSize += 1; //id
        }

        //characteristic statistics
        if( m_Verbosity == 0 ) //default, only min and max
        {
            indexSize += 2 * ( sizeof(T) + 1 );
            indexSize += 1 + 1; //id
        }

        return indexSize + 12; ///extra 12 bytes in case of attributes
        //need to add transform characteristics
    }

    /**
     * Version that takes directly a single Heap buffer class and a single BP1MetadataSet
     * Skip virtual if optimized to an object
     * @param group
     * @param variableName
     * @param variable
     * @param buffer
     * @param metadataSet
     */
    template< class T >
    void WriteVariableIndex( const Variable<T>& variable, capsule::STLVector& buffer, BP1MetadataSet& metadataSet ) const noexcept
    {
        // adapt this part to local variables
        std::vector<char*> dataBuffers{ buffer.m_Data.data() };
        std::vector<size_t> dataPositions { buffer.m_DataPosition };
        std::vector<size_t> dataAbsolutePositions { buffer.m_DataAbsolutePosition };

        std::vector<char*> metadataBuffers{ metadataSet.VarsIndex.data() };
        std::vector<std::size_t> metadataPositions{ metadataSet.VarsIndexPosition };
        std::vector<unsigned int> variablesCount{ metadataSet.VarsCount };

        WriteVariableIndexCommon( variable, dataBuffers, dataPositions, dataAbsolutePositions,
                                  metadataBuffers, metadataPositions, variablesCount );

        //update positions and varsCount originally passed by value
        buffer.m_DataPosition = dataPositions[0];
        buffer.m_DataAbsolutePosition = dataAbsolutePositions[0];
        metadataSet.VarsIndexPosition = metadataPositions[0];
        metadataSet.VarsCount += 1;
    }


    /**
     * Version that writes to a vector of capsules
     * @param group variable owner
     * @param variableName name of variable to be written
     * @param variable object carrying variable information
     * @param capsules from Engine member m_Capsules
     * @param metadataSets
     */
    template< class T >
    void WriteVariableIndex( const Variable<T>& variable,
                             std::vector< std::shared_ptr<Capsule> >& capsules,
                             std::vector<BP1MetadataSet>& metadataSets ) const noexcept
    {
        // adapt this part to local variables
        std::vector<char*> metadataBuffers, dataBuffers;
        std::vector<std::size_t> metadataPositions, dataPositions, dataAbsolutePositions;
        std::vector<unsigned int> variablesCount;

        for( auto& metadataSet : metadataSets )
        {
            metadataBuffers.push_back( metadataSet.VarsIndex.data() );
            metadataPositions.push_back( metadataSet.VarsIndexPosition );
            variablesCount.push_back( metadataSet.VarsCount );
        }

        for( auto& capsule : capsules )
        {
            dataBuffers.push_back( capsule->GetData( ) );
            dataPositions.push_back( capsule->m_DataPosition );
            dataAbsolutePositions.push_back( capsule->m_DataAbsolutePosition );
        }

        WriteVariableIndexCommon( variable, dataBuffers, dataPositions, dataAbsolutePositions,
                                  metadataBuffers, metadataPositions, variablesCount );

        //update positions and varsCount originally passed by value
        const unsigned int buffersSize = static_cast<unsigned int>( capsules.size() );
        for( unsigned int i = 0; i < buffersSize; ++i )
        {
            metadataSets[i].VarsIndexPosition = metadataPositions[i];
            metadataSets[i].VarsCount += 1;

            capsules[i]->m_DataPosition = dataPositions[i];
            capsules[i]->m_DataAbsolutePosition = dataAbsolutePositions[i];
        }
    }

    /**
     * Expensive part this is only for heap buffers need to adapt to vector of capsules
     * @param variable
     * @param buffer
     */
    template< class T >
    void WriteVariablePayload( const Variable<T>& variable, capsule::STLVector& buffer, const unsigned int cores = 1 ) const noexcept
    {
        std::size_t payloadSize = variable.PayLoadSize();
        MemcpyThreads( buffer.m_Data.data(), variable.m_AppValues, payloadSize, cores ); //EXPENSIVE part, might want to use threads if large.
        //update indices
        buffer.m_DataPosition += payloadSize;
        buffer.m_DataAbsolutePosition += payloadSize;
    }


    /**
     * Function that sets metadata (if first close) and writes to a single transport
     * @param metadataSet current rank metadata set
     * @param capsule contains data and metadata buffers
     * @param transport does a write after data and metadata is setup
     * @param isFirstClose true: metadata has been set and aggregated
     * @param haveMetadata true: attach metadata buffer to each data buffer and do a transport write
     * @param haveTiming true: add timing.log file
     */
    void Close( BP1MetadataSet& metadataSet, Capsule& capsule, Transport& transport, bool& isFirstClose,
    		    const bool haveMetadata = true, const bool haveTiming = false ) const noexcept;


private:

    /**
     * Common function that Writes a process group index PGIndex, done at Open or aggregation of new time step.
     * Called from public WriteProcessGroupIndex functions.
     * @param isFortran true: using Fortran, false: other language
     * @param name process group, usually the rank (maybe communicator?)
     * @param processID processID, usually the rank
     * @param timeStepName
     * @param timeStep
     * @param dataBuffers
     * @param dataPositions
     * @param dataAbsolutePositions
     * @param metadataBuffers
     * @param metadataPositions
     */
    void WriteProcessGroupIndexCommon( const bool isFortran, const std::string name, const unsigned int processID,
                                       const std::string timeStepName, const unsigned int timeStep,
                                       const std::vector<int>& methodIDs,
                                       std::vector<char*>& dataBuffers, std::vector<std::size_t>& dataPositions,
                                       std::vector<std::size_t>& dataAbsolutePositions,
                                       std::vector<char*>& metadataBuffers,
                                       std::vector<std::size_t>& metadataPositions ) const noexcept;

    /**
     *
     * @param variable
     * @param dataBuffers
     * @param dataPositions
     * @param dataAbsolutePositions
     * @param metadataBuffers
     * @param metadataPositions
     * @param variablesCount
     */
    template<class T>
    void WriteVariableIndexCommon( const Variable<T>& variable,
                                   std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                                   std::vector<size_t>& dataAbsolutePositions,
                                   std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions,
                                   std::vector<unsigned int> variablesCount ) const noexcept
    {
        //capture initial positions
        std::vector<std::size_t> metadataLengthPositions( metadataPositions );
        std::vector<std::size_t> dataLengthPositions( dataPositions );

        MovePositions( 4, metadataPositions ); //length of var, will come at the end from this offset
        MovePositions( 8, dataPositions ); //length of var, will come at the end from this offset

        //memberID
        MemcpyToBuffers( metadataBuffers, metadataPositions, variablesCount, 4 );
        //skipping 2 bytes for length of group name which is zero, only in metadata
        MovePositions( 2, metadataPositions ); //length of var, will come at the end from this offset

        //variable name to metadata and data
        const std::uint16_t lengthVariableName = variable.m_Name.length();
        WriteNameRecord( variable.m_Name, lengthVariableName, metadataBuffers, metadataPositions );
        WriteNameRecord( variable.m_Name, lengthVariableName, dataBuffers, dataPositions );

        //skip path (jump 2 bytes, already set to zero)
        MovePositions( 2, metadataPositions ); //length of var, will come at the end from this offset
        MovePositions( 2, dataPositions ); //length of var, will come at the end from this offset

        //dataType
        const std::uint8_t dataType = GetDataType<T>();
        MemcpyToBuffers( metadataBuffers, metadataPositions, &dataType, 1 );
        MemcpyToBuffers( dataBuffers, dataPositions, &dataType, 1 );

        //Characteristics Sets in Metadata and Data
        //const std::vector<std::size_t> metadataCharacteristicsSetsCountPosition( metadataPositions ); //very important piece
        std::vector<std::size_t> dataCharacteristicsCountPositions( dataPositions ); //very important piece

        const std::uint64_t sets = 1; //write one for now
        MemcpyToBuffers( metadataBuffers, metadataPositions, &sets, 8 );
        std::vector<std::size_t> metadataCharacteristicsCountPositions( metadataPositions ); //very important, can't be const as it is updated by MemcpyToBuffer

        std::uint8_t characteristicsCounter = 0; //used for characteristics count, characteristics length will be calculated at the end
        //here move positions 5 bytes in data and metadata for characteristics count + length
        MovePositions( 5, metadataPositions );

        //DIMENSIONS CHARACTERISTIC
        const std::vector<std::size_t>& localDimensions = variable.m_Dimensions;

        //write to metadata characteristic
        //characteristic: dimension
        std::uint8_t characteristicID = characteristic_dimensions;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 );
        const std::uint8_t dimensions = localDimensions.size();
        MemcpyToBuffers( metadataBuffers, metadataPositions, &dimensions, 1 );
        const std::uint16_t dimensionsLength = dimensions * 24; //24 is from 8 bytes for each: local dimension, global dimension, global offset
        MemcpyToBuffers( metadataBuffers, metadataPositions, &dimensionsLength, 2 );

        //write in data if it's a dimension variable (scalar) y or n
        const char dimensionYorN = ( variable.m_IsDimension ) ? 'y' : 'n';
        MemcpyToBuffers( dataBuffers, dataPositions, &dimensionYorN, 1 );

        MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
        const std::uint16_t dimensionsLengthInData = dimensions * 27; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset
        MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLengthInData, 2 );

        if( variable.m_GlobalDimensions.empty() ) //local variable
        {
            WriteDimensionRecord( metadataBuffers, metadataPositions, localDimensions, 16 );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, 18, true ); //not using memberID for now

            dataCharacteristicsCountPositions = dataPositions; //very important to track as writer is going back to this position
            MovePositions( 5, dataPositions ); //skip characteristics count(1) + length (4)

            //dimensions in data characteristic entry
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 );
            const std::int16_t lengthOfDimensionsCharacteristic = 24 * dimensions; // 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLength, 2 );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, 16 );
        }
        else //global variable
        {
            const std::vector<std::size_t>& globalDimensions = variable.m_GlobalDimensions;
            const std::vector<std::size_t>& globalOffsets = variable.m_GlobalOffsets;

            WriteDimensionRecord( metadataBuffers, metadataPositions, localDimensions, globalDimensions, globalOffsets );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, globalDimensions, globalOffsets, true );

            dataCharacteristicsCountPositions = dataPositions; //very important, going back to these positions
            MovePositions( 5, dataPositions ); //skip characteristics count(1) + length (4)

            //dimensions in data characteristic entry
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 ); //id
            const std::int16_t lengthOfDimensionsCharacteristic = 24 * dimensions; // 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLength, 2 );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, globalDimensions, globalOffsets );
        }
        ++characteristicsCounter;

        //VALUE for SCALAR or STAT min, max for ARRAY
        //Value for scalar
        if( variable.m_IsScalar ) //scalar //just doing string scalars for now (by name), needs to be modified when user passes value
        {
            characteristicID = characteristic_value;
            const std::int16_t lengthOfName = variable.m_Name.length();
            //metadata
            MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1  );
            WriteNameRecord( variable.m_Name, lengthOfName, metadataBuffers, metadataPositions );

            //data
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfName, 2 ); //add length of characteristic in data
            WriteNameRecord( variable.m_Name, lengthOfName, dataBuffers, dataPositions );

            ++characteristicsCounter;
        }
        else // Stat -> Min, Max for arrays,
        {
            if( m_Verbosity == 0 ) //default verbose
            {
                WriteMinMax( variable, dataBuffers, dataPositions, metadataBuffers, metadataPositions );
                characteristicsCounter += 2;
            }
        }

        //Back to characteristics count and length in Data
        std::vector<std::uint32_t> dataCharacteristicsLengths( dataPositions.size() );
        for( unsigned int i = 0; i < dataPositions.size(); ++i )
            dataCharacteristicsLengths[i] = dataPositions[i] - dataCharacteristicsCountPositions[i] - 4 - 1; //remove its own length (4 bytes) + characteristic counter

        MemcpyToBuffers( dataBuffers, dataCharacteristicsCountPositions, &characteristicsCounter, 1 );
        MemcpyToBuffers( dataBuffers, dataCharacteristicsCountPositions, dataCharacteristicsLengths, 4 ); //vector to vector

        //Metadata only: Offsets should be last, they come from data absolute positions
        characteristicID = characteristic_offset;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //variable offset id
        MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 ); //variable offset
        ++characteristicsCounter;

        //update absolute positions with dataPositions, this is the payload offset
        std::vector<std::uint64_t> varLengths( dataPositions.size() );
        for( unsigned int i = 0; i < dataAbsolutePositions.size(); ++i )
        {
        	varLengths[i] = dataPositions[i] - dataLengthPositions[i];
        	dataAbsolutePositions[i] += varLengths[i];
        	varLengths[i] += variable.PayLoadSize() - 8; //remove its own size
        }

        characteristicID = characteristic_payload_offset;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //variable payload offset id
        MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 ); //variable payload offset
        ++characteristicsCounter;

        //Back to writing characteristics count and length in Metadata
        std::vector<std::uint32_t> metadataCharacteristicsLengths( metadataPositions.size() );
        for( unsigned int i = 0; i < metadataPositions.size(); ++i )
            metadataCharacteristicsLengths[i] = metadataPositions[i] - metadataCharacteristicsCountPositions[i] - 4 - 1; //remove its own size and characteristic counter size

        MemcpyToBuffers( metadataBuffers, metadataCharacteristicsCountPositions, &characteristicsCounter, 1 );
        MemcpyToBuffers( metadataBuffers, metadataCharacteristicsCountPositions, metadataCharacteristicsLengths, 4 ); //vector to vector

        //Back to var entry length
        std::vector<std::uint32_t> metadataVarEntryLengths( metadataPositions.size() );
        for( unsigned int i = 0; i < metadataPositions.size(); ++i )
        	metadataVarEntryLengths[i] = metadataPositions[i] - metadataLengthPositions[i] - 4; //remove its own size

        MemcpyToBuffers( metadataBuffers, metadataLengthPositions, metadataVarEntryLengths, 4 ); //vector to vector

        //Need to add length of var including payload size
        MemcpyToBuffers( dataBuffers, dataLengthPositions, varLengths, 8 );
    }


    /**
     * Writes name record using a
     * @param name to be written
     * @param length number of characters in name
     * @param buffers to be written
     * @param positions to be moved
     */
    void WriteNameRecord( const std::string name, const std::uint16_t length,
                          std::vector<char*>& buffers, std::vector<std::size_t>& positions ) const noexcept;

    /**
     * Write a dimension record for a global variable used by WriteVariableCommon
     * @param buffers
     * @param positions
     * @param localDimensions
     * @param globalDimensions
     * @param globalOffsets
     * @param addType true: for data buffers, false: for metadata buffer and data characteristic
     */
    void WriteDimensionRecord( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                               const std::vector<std::size_t>& localDimensions,
                               const std::vector<std::size_t>& globalDimensions,
                               const std::vector<std::size_t>& globalOffsets,
                               const bool addType = false ) const noexcept;

    /**
     * Write a dimension record for a local variable used by WriteVariableCommon
     * @param buffers
     * @param positions
     * @param localDimensions
     * @param skip
     * @param addType true: for data buffers, false: for metadata buffer and data characteristic
     */
    void WriteDimensionRecord( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                               const std::vector<std::size_t>& localDimensions,
                               const unsigned int skip,
                               const bool addType = false ) const noexcept;

    /**
     * Function that writes min and max into data and metadata, called from WriteVariableIndex common.
     * Will be specialized for complex types, this is the version for primitive types
     * @param variable
     * @param dataBuffers
     * @param dataPositions
     * @param metadataBuffers
     * @param metadataPositions
     */
    template<class T> inline
    void WriteMinMax( const Variable<T>& variable,
                      std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                      std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions ) const noexcept
    {
        T min, max;
        const std::size_t valuesSize = variable.TotalSize();
        if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
            GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
        else
            GetMinMax( variable.m_AppValues, valuesSize, min, max );

        WriteMinMaxValues( min, max, dataBuffers, dataPositions, metadataBuffers, metadataPositions );
    }


    /**
     * Common part of WriteMinMax specialized templates. Writes to buffers after min and max are calculated.
     */
    template<class T>
    void WriteMinMaxValues( const T min, const T max,
                            std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                            std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions ) const noexcept
    {
        constexpr std::int8_t characteristicMinID = characteristic_min;
        constexpr std::int8_t characteristicMaxID = characteristic_max;

        WriteValueRecord( characteristicMinID, min, metadataBuffers, metadataPositions );
        WriteValueRecord( characteristicMaxID, max, metadataBuffers, metadataPositions );
        WriteValueRecord( characteristicMinID, min, dataBuffers, dataPositions, true ); //addLength in between for data
        WriteValueRecord( characteristicMaxID, max, dataBuffers, dataPositions, true ); //addLength in between for data
    }


    /**
     * Write a statistics record to buffer
     * @param id
     * @param value
     * @param buffers
     * @param positions
     * @param addLength true for data, false for metadata
     */
    template<class T>
    void WriteValueRecord( const std::uint8_t& characteristicID, const T& value,
                           std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                           const bool addLength = false ) const noexcept
    {
        MemcpyToBuffers( buffers, positions, &characteristicID, 1 );

        if( addLength == true )
        {
            const std::int16_t lengthCharacteristic = 1 + sizeof( T );
            MemcpyToBuffers( buffers, positions, &lengthCharacteristic, 2 );
        }

        MemcpyToBuffers( buffers, positions, &value, sizeof(T) );
    }

    /**
     * Flattens the metadata indices into a single metadata buffer in capsule
     * @param metadataSet
     * @param capsule
     */
    void FlattenMetadata( BP1MetadataSet& metadataSet, Capsule& capsule ) const noexcept; ///< sets the metadata buffer in capsule with indices and minifooter


    /**
     * Flattens the data and fills the pg length, vars count, vars length and attributes
     * @param metadataSet
     * @param capsule
     */
    void FlattenData( BP1MetadataSet& metadataSet, Capsule& capsule ) const noexcept;

};



/**
 * Specilized version of WriteMinMax for std::complex<float>
 * @param variable
 * @param dataBuffers
 * @param dataPositions
 * @param metadataBuffers
 * @param metadataPositions
 */
template<> inline
void BP1Writer::WriteMinMax<std::complex<float>>( const Variable<std::complex<float>>& variable,
                                                  std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                                                  std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions ) const noexcept
{
    float min, max;
    const std::size_t valuesSize = variable.TotalSize();
    if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
        GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
    else
        GetMinMax( variable.m_AppValues, valuesSize, min, max );

    WriteMinMaxValues( min, max, dataBuffers, dataPositions, metadataBuffers, metadataPositions );
}


template<> inline
void BP1Writer::WriteMinMax<std::complex<double>>( const Variable<std::complex<double>>& variable,
                                                  std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                                                  std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions ) const noexcept
{
    double min, max;
    const std::size_t valuesSize = variable.TotalSize();
    if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
        GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
    else
        GetMinMax( variable.m_AppValues, valuesSize, min, max );

    WriteMinMaxValues( min, max, dataBuffers, dataPositions, metadataBuffers, metadataPositions );
}


template<> inline
void BP1Writer::WriteMinMax<std::complex<long double>>( const Variable<std::complex<long double>>& variable,
                                                        std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                                                        std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions ) const noexcept
{
    long double min, max;
    const std::size_t valuesSize = variable.TotalSize();
    if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
        GetMinMax( variable.m_AppValues, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
    else
        GetMinMax( variable.m_AppValues, valuesSize, min, max );

    WriteMinMaxValues( min, max, dataBuffers, dataPositions, metadataBuffers, metadataPositions );
}




} //end namespace format
} //end namespace adios

#endif /* BP1WRITER_H_ */
