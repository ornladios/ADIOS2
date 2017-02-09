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
#include <cstdint>
#include <algorithm> //std::count, std::copy, std::for_each
#include <cstring> //std::memcpy
#include <cmath>   //std::ceil
/// \endcond

#include "BP1.h"
#include "core/Variable.h"
#include "core/Group.h"
#include "core/Capsule.h"
#include "capsule/Heap.h"
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
    unsigned int m_VariablesTotalCount = 0;

    /**
     * Calculates the Process Index size in bytes according to the BP format
     * @param name process group name
     * @param timeStepName name of the corresponding time step
     * @return size of process group index in bytes
     */
    std::size_t GetProcessIndexSize( const std::string name, const std::string timeStepName );

    /**
     * Writes a PGIndex, done at Open or aggregation
     * @param isFortran
     * @param name
     * @param processID
     * @param timeStepName
     * @param timeStep
     * @param dataBuffers
     * @param dataPositions
     * @param dataAbsolutePositions
     * @param metadataBuffers
     * @param metadataPositions
     */
    void WriteProcessGroupIndex( const bool isFortran, const std::string name, const unsigned int processID,
                                 const std::string timeStepName, const unsigned int timeStep,
                                 std::vector<char*>& dataBuffers, std::vector<std::size_t>& dataPositions,
                                 std::vector<std::size_t>& dataAbsolutePositions,
                                 std::vector<char*>& metadataBuffers,
                                 std::vector<std::size_t>& metadataPositions );

    /**
     * Returns the estimated variable index size
     * @param group
     * @param variableName
     * @param variable
     * @param verbosity
     * @return variable index size
     */
    template< class T >
    size_t GetVariableIndexSize( const Group& group, const std::string variableName,
                                 const Variable<T> variable ) noexcept
    {
        //size_t indexSize = varEntryLength + memberID + lengthGroupName + groupName + lengthVariableName + lengthOfPath + path + datatype
        size_t indexSize = 23; //without characteristics
        indexSize += group.m_Name.size();
        indexSize += variableName.size();

        // characteristics 3 and 4, check variable number of dimensions
        const std::size_t dimensions = std::count( variable.DimensionsCSV.begin(), variable.DimensionsCSV.end(), ',' ) + 1; //number of commas in CSV + 1
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
            indexSize += 2 + variableName.size();
            indexSize += 1; //id
        }

        //characteristic statistics
        if( m_Verbosity == 0 ) //default, only min and max
        {
            indexSize += 2 * ( sizeof(T) + 1 );
            indexSize += 1 + 1; //id
        }

        return indexSize;
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
    void WriteVariableIndex( const Group& group, const Var variableName, const Variable<T>& variable,
                             Heap& buffer, BP1MetadataSet& metadataSet ) noexcept
    {
        // adapt this part to local variables
        std::vector<char*> dataBuffers{ buffer.m_Data.data() };
        std::vector<size_t> dataPositions { buffer.m_DataPosition };
        std::vector<size_t> dataAbsolutePositions { buffer.m_DataAbsolutePosition };

        std::vector<char*> metadataBuffers{ metadataSet.VarsIndex.data() };
        std::vector<std::size_t> metadataPositions{ metadataSet.VarsIndexPosition };
        std::vector<unsigned int> variablesCount{ metadataSet.VarsCount };

        WriteVariableCommon( group, variableName, variable, dataBuffers, dataPositions, dataAbsolutePositions,
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
    void WriteVariableIndex( const Group& group, const Var variableName, const Variable<T>& variable,
                             std::vector< std::shared_ptr<Capsule> >& capsules,
                             std::vector<BP1MetadataSet>& metadataSets ) noexcept
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

        WriteVariableCommon( group, variableName, variable, dataBuffers, dataPositions, dataAbsolutePositions,
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

    void Close( const BP1MetadataSet& metadataSet, Capsule& capsule, Transport& transport );


private:


    template<class T>
    void WriteVariableCommon( const Group& group, const Var variableName, const Variable<T>& variable,
                              std::vector<char*>& dataBuffers, std::vector<size_t>& dataPositions,
                              std::vector<size_t>& dataAbsolutePositions,
                              std::vector<char*>& metadataBuffers, std::vector<size_t>& metadataPositions,
                              std::vector<unsigned int> variablesCount )
    {
        //capture initial positions
        const std::vector<std::size_t> metadataLengthPositions( metadataPositions );
        const std::vector<std::size_t> dataLengthPositions( dataPositions );

        MovePositions( 4, metadataPositions ); //length of var, will come at the end from this offset
        MovePositions( 8, dataPositions ); //length of var, will come at the end from this offset

        //memberID
        MemcpyToBuffers( metadataBuffers, metadataPositions, variablesCount, 4 );
        //group name, only in metadata
        const std::uint16_t lengthGroupName = group.m_Name.length();
        WriteNameRecord( group.m_Name, lengthGroupName, metadataBuffers, metadataPositions );

        //variable name to metadata and data
        const std::uint16_t lengthVariableName = variableName.length();
        WriteNameRecord( variableName, lengthVariableName, metadataBuffers, metadataPositions );
        WriteNameRecord( variableName, lengthVariableName, dataBuffers, dataPositions );

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

        //DIMENSIONS CHARACTERISTIC
        const std::vector<unsigned long long int> localDimensions = group.GetDimensions( variable.DimensionsCSV );

        //write to metadata characteristic
        //characteristic: dimension
        std::uint8_t characteristicID = characteristic_dimensions;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 );
        const std::uint8_t dimensions = localDimensions.size();
        MemcpyToBuffers( metadataBuffers, metadataPositions, &dimensions, 1 );
        const std::uint16_t dimensionsLength = dimensions * 24; //24 is from 8 bytes for each: local dimension, global dimension, global offset
        MemcpyToBuffers( metadataBuffers, metadataPositions, &dimensionsLength, 2 );

        //write in data if it's a dimension variable (scalar) y or n
        const char dimensionYorN = ( variable.IsDimension ) ? 'y' : 'n';
        MemcpyToBuffers( dataBuffers, dataPositions, &dimensionYorN, 1 );
        MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
        const std::uint16_t dimensionsLengthInData = dimensions * 27; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset
        MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLengthInData, 2 );

        if( variable.GlobalBoundsIndex == -1 ) //local variable
        {
            WriteDimensionRecord( metadataBuffers, metadataPositions, localDimensions, 16 );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, 18, true ); //not using memberID for now

            dataCharacteristicsCountPositions = dataPositions; //very important to track as writer is going back to this position
            MovePositions( 5, dataPositions ); //skip characteristics count(1) + length (4)

            //dimensions in data characteristic entry
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 );
            const std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLength, 2 );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, 16 );
        }
        else //global variable
        {
            const std::vector<unsigned long long int> globalDimensions = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].first );
            const std::vector<unsigned long long int> globalOffsets = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].second );

            WriteDimensionRecord( metadataBuffers, metadataPositions, localDimensions, globalDimensions, globalOffsets );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, globalDimensions, globalOffsets, true );

            dataCharacteristicsCountPositions = dataPositions; //very important, going back to these positions
            MovePositions( 5, dataPositions ); //skip characteristics count(1) + length (4)

            //dimensions in data characteristic entry
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 ); //id
            const std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLength, 2 );
            WriteDimensionRecord( dataBuffers, dataPositions, localDimensions, globalDimensions, globalOffsets );
        }
        ++characteristicsCounter;

        //VALUE for SCALAR or STAT min, max for ARRAY
        //Value for scalar
        if( variable.DimensionsCSV == "1" ) //scalar //just doing string scalars for now (by name), needs to be modified when user passes value
        {
            characteristicID = characteristic_value;
            const std::int16_t lengthOfName = variableName.length();
            //metadata
            MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1  );
            WriteNameRecord( variableName, lengthOfName, metadataBuffers, metadataPositions );

            //data
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 );
            const std::int16_t lengthOfCharacteristic = 2 + lengthOfName;
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfCharacteristic, 2 ); //added in data
            WriteNameRecord( variableName, lengthOfName, dataBuffers, dataPositions );
        }
        else // Stat -> Min, Max for arrays,
        {
            if( m_Verbosity == 0 ) //default verbose
            {
                //Get min and max
                const std::size_t valuesSize = GetTotalSize( localDimensions );
                T min, max;

                if( valuesSize >= 10000000 ) //ten million? this needs actual results //here we can make decisions for threads based on valuesSize
                    GetMinMax( variable.Values, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
                else
                    GetMinMax( variable.Values, valuesSize, min, max );

                //set characteristic ids for min and max
                characteristicID = characteristic_stat;
                constexpr std::int8_t statisticMinID = statistic_min;
                constexpr std::int8_t statisticMaxID = statistic_max;

                WriteStatisticsRecord( statisticMinID, min, metadataBuffers, metadataPositions );
                WriteStatisticsRecord( statisticMaxID, max, metadataBuffers, metadataPositions );
                WriteStatisticsRecord( statisticMinID, min, dataBuffers, dataPositions, true ); //addLength in between
                WriteStatisticsRecord( statisticMaxID, max, dataBuffers, dataPositions, true ); //addLength in between
            }
        }
        ++characteristicsCounter;

        //Back to count and length in Data
        std::vector<std::uint32_t> dataCharacteristicsLengths( dataPositions.size() );
        for( unsigned int i = 0; i < dataPositions.size(); ++i )
            dataCharacteristicsLengths[i] = dataPositions[i] - dataCharacteristicsCountPositions[i];

        MemcpyToBuffers( dataBuffers, dataCharacteristicsCountPositions, &characteristicsCounter, 1 );
        MemcpyToBuffers( dataBuffers, dataCharacteristicsCountPositions, dataCharacteristicsLengths, 4 ); //vector to vector
        MovePositions( -5, dataCharacteristicsCountPositions ); //back to original position

        //Offsets should be last and only written to metadata, they come from absolute positions
        characteristicID = characteristic_offset;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //variable offset id
        MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 ); //variable offset
        ++characteristicsCounter;

        //update absolute positions with dataPositions, this is the payload offset
        for( unsigned int i = 0; i < dataAbsolutePositions.size(); ++i )
            dataAbsolutePositions[i] += dataPositions[i];

        characteristicID = characteristic_payload_offset;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //variable payload offset id
        MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 ); //variable payload offset
        ++characteristicsCounter;

        //Back to writing characteristics count and length in Metadata
        std::vector<std::uint32_t> metadataCharacteristicsLengths( metadataPositions.size() );
        for( unsigned int i = 0; i < metadataPositions.size(); ++i )
            metadataCharacteristicsLengths[i] = metadataPositions[i] - metadataCharacteristicsCountPositions[i];

        MemcpyToBuffers( metadataBuffers, metadataCharacteristicsCountPositions, &characteristicsCounter, 1 );
        MemcpyToBuffers( metadataBuffers, metadataCharacteristicsCountPositions, metadataCharacteristicsLengths, 4 ); //vector to vector
        MovePositions( -5, metadataCharacteristicsCountPositions ); //back to original position

        ++m_VariablesTotalCount;
    } //end of function


    /**
     * Writes name record using a
     * @param name to be written
     * @param length number of characters in name
     * @param buffers to be written
     * @param positions to be moved
     */
    void WriteNameRecord( const std::string name, const std::uint16_t length,
                          std::vector<char*>& buffers, std::vector<std::size_t>& positions );

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
                               const std::vector<unsigned long long int>& localDimensions,
                               const std::vector<unsigned long long int>& globalDimensions,
                               const std::vector<unsigned long long int>& globalOffsets,
                               const bool addType = false );

    /**
     * Write a dimension record for a local variable used by WriteVariableCommon
     * @param buffers
     * @param positions
     * @param localDimensions
     * @param skip
     * @param addType true: for data buffers, false: for metadata buffer and data characteristic
     */
    void WriteDimensionRecord( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                               const std::vector<unsigned long long int>& localDimensions,
                               const unsigned int skip,
                               const bool addType = false );

    /**
     *
     * @param id
     * @param value
     * @param buffers
     * @param positions
     * @param addLength true for data, false for metadata
     */
    template<class T>
    void WriteStatisticsRecord( const std::uint8_t& id, const T& value,
                                std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                                const bool addLength = false )
    {
        const std::uint8_t characteristicID = characteristic_stat;
        MemcpyToBuffers( buffers, positions, &characteristicID, 1 );

        if( addLength == true )
        {
            const std::int16_t lengthCharacteristic = 1 + sizeof( T );
            MemcpyToBuffers( buffers, positions, &lengthCharacteristic, 2 );
        }

        MemcpyToBuffers( buffers, positions, &id, 1 );
        MemcpyToBuffers( buffers, positions, &value, sizeof(T) );
    }

    /**
     * Returns data type index from enum Datatypes
     * @param variable input variable
     * @return data type
     */
    template< class T > inline std::int8_t GetDataType( ) noexcept { return type_unknown; }

    void CloseRankFile( Capsule& capsule, Transport& transport );

    void SetMetadata( const BP1MetadataSet& metadataSet, Capsule& capsule ); ///< sets the metadata buffer in capsule with indices and minifooter
    void SetMiniFooter( BP1MetadataSet& metadataSet ); ///< sets the minifooter

};


//Moving template BP1Writer::GetDataType template specializations outside of the class
template< > inline std::int8_t BP1Writer::GetDataType<char>( ) noexcept { return type_byte; }
template< > inline std::int8_t BP1Writer::GetDataType<short>( ) noexcept{ return type_short; }
template< > inline std::int8_t BP1Writer::GetDataType<int>( ) noexcept{ return type_integer; }
template< > inline std::int8_t BP1Writer::GetDataType<long int>( ) noexcept{ return type_long; }

template< > inline std::int8_t BP1Writer::GetDataType<unsigned char>( ) noexcept { return type_unsigned_byte; }
template< > inline std::int8_t BP1Writer::GetDataType<unsigned short>( ) noexcept{ return type_unsigned_short; }
template< > inline std::int8_t BP1Writer::GetDataType<unsigned int>( ) noexcept{ return type_unsigned_integer; }
template< > inline std::int8_t BP1Writer::GetDataType<unsigned long int>( ) noexcept{ return type_unsigned_long; }

template< > inline std::int8_t BP1Writer::GetDataType<float>( ) noexcept{ return type_real; }
template< > inline std::int8_t BP1Writer::GetDataType<double>( ) noexcept{ return type_double; }
template< > inline std::int8_t BP1Writer::GetDataType<long double>( ) noexcept{ return type_long_double; }



} //end namespace format
} //end namespace adios

#endif /* BP1WRITER_H_ */
