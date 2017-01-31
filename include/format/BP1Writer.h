/*
 * BP1.h
 *
 *  Created on: Jan 24, 2017
 *      Author: wfg
 */

#ifndef BP1WRITER_H_
#define BP1WRITER_H_


#include <vector>
#include <cstdint>
#include <algorithm> //std::count, std::copy, std::for_each
#include <cstring> //std::memcpy

#include "core/Variable.h"
#include "core/Group.h"



namespace adios
{
namespace format
{


class BP1Writer
{

public:

    std::uint64_t m_ProcessGroupsCount = 0; ///< number of process groups
    std::uint64_t m_ProcessGroupsLength = 0; ///< length in bytes of process groups
    std::vector<char> m_ProcessGroupIndex; ///< process group index metadata

    std::uint32_t m_VariablesCount = 0; ///< number of written Variables
    std::uint64_t m_VariablesLength = 0; ///< length in bytes of written Variables
    std::vector<char> m_VariableIndex; ///< metadata variable index

    std::uint32_t m_AttributesCount = 0; ///< number of Attributes
    std::uint64_t m_AttributesLength = 0; ///< length in bytes of Attributes
    std::vector<char> m_AttributeIndex; ///< metadata attribute index

    const unsigned int m_Cores = 1;
    const unsigned int m_Verbosity = 0;

    /**
     * DataTypes mapping in BP Format
     */
    enum DataTypes
    {
        type_unknown = -1,         //!< type_unknown
        type_byte = 0,             //!< type_byte
        type_short = 1,            //!< type_short
        type_integer = 2,          //!< type_integer
        type_long = 4,             //!< type_long

        type_unsigned_byte = 50,   //!< type_unsigned_byte
        type_unsigned_short = 51,  //!< type_unsigned_short
        type_unsigned_integer = 52,//!< type_unsigned_integer
        type_unsigned_long = 54,   //!< type_unsigned_long

        type_real = 5,             //!< type_real or float
        type_double = 6,           //!< type_double
        type_long_double = 7,      //!< type_long_double

        type_string = 9,           //!< type_string
        type_complex = 10,         //!< type_complex
        type_double_complex = 11,  //!< type_double_complex
        type_string_array = 12     //!< type_string_array
    };

    /**
     * Characteristic ID in variable metadata
     */
    enum VariableCharacteristicID
    {
        characteristic_value          = 0, //!< characteristic_value
        characteristic_min            = 1, //!< This is no longer used. Used to read in older bp file format
        characteristic_max            = 2, //!< This is no longer used. Used to read in older bp file format
        characteristic_offset         = 3, //!< characteristic_offset
        characteristic_dimensions     = 4, //!< characteristic_dimensions
        characteristic_var_id         = 5, //!< characteristic_var_id
        characteristic_payload_offset = 6, //!< characteristic_payload_offset
        characteristic_file_index     = 7, //!< characteristic_file_index
        characteristic_time_index     = 8, //!< characteristic_time_index
        characteristic_bitmap         = 9, //!< characteristic_bitmap
        characteristic_stat           = 10,//!< characteristic_stat
        characteristic_transform_type = 11 //!< characteristic_transform_type
    };


    /** Define statistics type for characteristic ID = 10 in bp1 format */
    enum VariableStatistics
    {
        statistic_min             = 0,
        statistic_max             = 1,
        statistic_cnt             = 2,
        statistic_sum             = 3,
        statistic_sum_square      = 4,
        statistic_hist            = 5,
        statistic_finite          = 6
    };


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
        std::size_t dimensions = std::count( variable.DimensionsCSV.begin(), variable.DimensionsCSV.end(), ',' ) + 1; //number of commas in CSV + 1
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
     * Write to many buffers and updates offset
     * @param buffers
     * @param source
     * @param size
     * @param offset
     */
    template< class T >
    void MemcpyToBuffers( std::vector<char*>& buffers, std::vector<std::size_t>& positions, const T* source, std::size_t size ) noexcept
    {
        const unsigned int length = buffers.size( );

        for( unsigned int i = 0; i < length; ++i )
        {
            std::memcpy( &buffers[positions[i]], source, size );
            positions[i] += size;
        }
    }

    template< class T >
    void MemcpyToBuffers( std::vector<char*>& buffers, std::vector<std::size_t>& positions,
                          const std::vector<T>& source, std::size_t size ) noexcept
    {
        const unsigned int length = buffers.size( );

        for( unsigned int i = 0; i < length; ++i )
        {
            std::memcpy( &buffers[positions[i]], &source[i], size );
            positions[i] += size;
        }
    }



    template< class T, class U >
    void CopyToBuffers( std::vector<char*>& buffers, std::vector<std::size_t>& positions, const T* source, U size ) noexcept
    {
        const unsigned int length = buffers.size( );

        for( unsigned int i = 0; i < length; ++i )
        {
            std::copy( source, source+size, &buffers[ positions[i] ] );
            positions[i] += size;
        }
    }


    template< class T, class U >
    void CopyToBuffers( std::vector<char*>& buffers, std::vector<std::size_t>& positions, const std::vector<T>& source, U size ) noexcept
    {
        const unsigned int length = buffers.size( );

        for( unsigned int i = 0; i < length; ++i )
        {
            std::copy( &source[i], &source[i]+size, &buffers[ positions[i] ] );
            positions[i] += size;
        }
    }

    /**
     * @param group variable owner
     * @param variableName name of variable to be written
     * @param variable object carrying variable information
     * @param dataBuffers buffers to which variable metadata and payload (values) will be written. Metadata is added in case of system corruption to allow regeneration.
     * @param dataPosition initial data relative position
     * @param dataAbsolutePosition data absolute position will be updated with dataPosition, needed for variable offset and variable payload offset
     * @param metadataBuffers buffers to which only variable metadata will be written
     * @param metadataPosition position in metadataBuffer
     */
    template< class T >
    void WriteVariable( const Group& group, const Var variableName, const Variable<T>& variable,
                        std::vector<char*>& dataBuffers,
                        std::vector<std::size_t>& dataPositions,
                        std::vector<std::size_t>& dataAbsolutePositions,
                        std::vector<char*>& metadataBuffers,
                        std::vector<std::size_t>& metadataPositions ) noexcept
    {
        auto lf_MovePositions = []( const int bytes, std::vector<std::size_t>& positions )
        {
            for( auto& position : positions ) // value or reference?
                position += bytes;
        };

        const std::vector<std::size_t> metadataLengthPositions( metadataPositions );
        const std::vector<std::size_t> dataLengthPositions( dataPositions );

        lf_MovePositions( 4, metadataPositions ); //length of var, will come at the end from this offset
        lf_MovePositions( 8, dataPositions ); //length of var, will come at the end from this offset

        //memberID
        MemcpyToBuffers( metadataBuffers, metadataPositions, &m_VariablesCount, 4 );
        //group, only in metadata
        const std::uint16_t lengthGroupName = group.m_Name.length();
        MemcpyToBuffers( metadataBuffers, metadataPositions, &lengthGroupName, 2 ); //2 bytes
        MemcpyToBuffers( metadataBuffers, metadataPositions, group.m_Name.c_str(), lengthGroupName );

        //variable name to metadata and data
        const std::uint16_t lengthVariableName = variableName.length();
        MemcpyToBuffers( metadataBuffers, metadataPositions, &lengthVariableName, 2 );
        MemcpyToBuffers( metadataBuffers, metadataPositions, variableName.c_str(), lengthVariableName );
        MemcpyToBuffers( dataBuffers, dataPositions, &lengthVariableName, 2 );
        MemcpyToBuffers( dataBuffers, dataPositions, variableName.c_str(), lengthVariableName );

        //skip path (jump 2 bytes, already set to zero)
        lf_MovePositions( 2, metadataPositions ); //length of var, will come at the end from this offset
        lf_MovePositions( 2, dataPositions ); //length of var, will come at the end from this offset

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
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                //metadata characteristics
                MemcpyToBuffers( metadataBuffers, metadataPositions, &localDimensions[d], 8 );
                lf_MovePositions( 16, metadataPositions ); //skipping global dimension(8), global offset (8)
            }

            const char no = 'n'; //dimension format unsigned int value (not using memberID for now)
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                //data dimensions
                MemcpyToBuffers( dataBuffers, dataPositions, &no, 1 );
                MemcpyToBuffers( dataBuffers, dataPositions, &localDimensions[d], 8 );
                lf_MovePositions( 18, dataPositions ); //skipping var no + global dimension(8), var no + global offset (8)
            }

            //dimensions in data characteristic entry
            dataCharacteristicsCountPositions = dataPositions; //very important

            lf_MovePositions( 5, dataPositions ); //skip characteristics count(1) + length (4)
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 );

            const std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLength, 2 );

            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                MemcpyToBuffers( dataBuffers, dataPositions, &localDimensions[d], 8 );
                lf_MovePositions( 16, dataPositions );
            }
        }
        else //global variable
        {
            const std::vector<unsigned long long int> globalDimensions = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].first );
            const std::vector<unsigned long long int> globalOffsets = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].second );

            //metadata, writing in characteristics
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                MemcpyToBuffers( metadataBuffers, metadataPositions, &localDimensions[d], 8 );
                MemcpyToBuffers( metadataBuffers, metadataPositions, &globalDimensions[d], 8 );
                MemcpyToBuffers( metadataBuffers, metadataPositions, &globalOffsets[d], 8 );
            }

            //data dimensions entry
            const char no = 'n'; //dimension format unsigned int value
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                MemcpyToBuffers( dataBuffers, dataPositions, &no, 1 );
                MemcpyToBuffers( dataBuffers, dataPositions, &localDimensions[d], 8 );
                MemcpyToBuffers( dataBuffers, dataPositions, &no, 1 );
                MemcpyToBuffers( dataBuffers, dataPositions, &localDimensions[d], 8 );
            }

            //dimensions in data characteristic entry
            dataCharacteristicsCountPositions = dataPositions;
            lf_MovePositions( 5, dataPositions ); //skip characteristics count(1) + length (4)
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 ); //id

            const std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfDimensionsCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensions, 1 );
            MemcpyToBuffers( dataBuffers, dataPositions, &dimensionsLength, 2 );

            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                MemcpyToBuffers( dataBuffers, dataPositions, &localDimensions[d], 8 );
                MemcpyToBuffers( dataBuffers, dataPositions, &globalDimensions[d], 8 );
                MemcpyToBuffers( dataBuffers, dataPositions, &globalOffsets[d], 8 );
            }
        }
        ++characteristicsCounter;

        //VALUE for SCALAR or STAT min, max for ARRAY
        //Value for scalar
        if( variable.DimensionsCSV == "1" ) //scalar //just doing string scalars for now (by name), needs to be modified when user passes value
        {
            characteristicID = characteristic_value;
            const std::int16_t lenghtOfName = variableName.length();
            //metadata
            MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1  );
            MemcpyToBuffers( metadataBuffers, metadataPositions, &lenghtOfName, 2 );
            MemcpyToBuffers( metadataBuffers, metadataPositions, variableName.c_str(), lenghtOfName );

            //data
            MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 );
            const std::int16_t lengthOfCharacteristic = 2 + lenghtOfName;
            MemcpyToBuffers( dataBuffers, dataPositions, &lengthOfCharacteristic, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, &lenghtOfName, 2 );
            MemcpyToBuffers( dataBuffers, dataPositions, variableName.c_str(), lenghtOfName );
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
                const std::int8_t statisticMinID = statistic_min;
                const std::int8_t statisticMaxID = statistic_max;

                //write min and max to metadata
                MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //min
                MemcpyToBuffers( metadataBuffers, metadataPositions, &statisticMinID, 1 );
                MemcpyToBuffers( metadataBuffers, metadataPositions, &min, sizeof(T) );

                MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1  ); //max
                MemcpyToBuffers( metadataBuffers, metadataPositions, &statisticMaxID, 1 );
                MemcpyToBuffers( metadataBuffers, metadataPositions, &max, sizeof(T) );

                //write min and max to data
                MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 ); //min
                const std::int16_t lengthCharacteristic = 1 + sizeof( T );
                MemcpyToBuffers( dataBuffers, dataPositions, &lengthCharacteristic, 2 );
                MemcpyToBuffers( dataBuffers, dataPositions, &statisticMinID, 1 );
                MemcpyToBuffers( dataBuffers, dataPositions, &min, sizeof(T) );

                MemcpyToBuffers( dataBuffers, dataPositions, &characteristicID, 1 ); //max
                MemcpyToBuffers( dataBuffers, dataPositions, &lengthCharacteristic, 2 );
                MemcpyToBuffers( dataBuffers, dataPositions, &statisticMaxID, 1 );
                MemcpyToBuffers( dataBuffers, dataPositions, &max, sizeof(T) );
            }
        }
        ++characteristicsCounter;

        //Characteristics count and length in Data
        std::vector<std::uint32_t> dataCharacteristicsLengths( dataPositions );
        std::transform( dataCharacteristicsLengths.begin( ), dataCharacteristicsLengths.end( ),
                        dataCharacteristicsCountPositions.begin(), dataCharacteristicsCountPositions.end(),
                        std::minus<std::uint32_t>() );

        MemcpyToBuffers( dataBuffers, dataCharacteristicsCountPositions, &characteristicsCounter, 1 );
        MemcpyToBuffers( dataBuffers, dataCharacteristicsCountPositions, dataCharacteristicsLengths, 4 ); //vector to vector
        lf_MovePositions( -5, dataCharacteristicsCountPositions ); //back to original position

        //Offsets should be last and only written to metadata, they come from absolute positions
        characteristicID = characteristic_offset;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //variable offset id
        MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 ); //variable offset
        ++characteristicsCounter;

        //update absolute positions with dataPositions, this is the payload offset
        std::transform( dataAbsolutePositions.begin(), dataAbsolutePositions.end(),
                        dataPositions.begin(), dataPositions.end(), std::plus<std::size_t>() );

        characteristicID = characteristic_payload_offset;
        MemcpyToBuffers( metadataBuffers, metadataPositions, &characteristicID, 1 ); //variable payload offset id
        MemcpyToBuffers( metadataBuffers, metadataPositions, dataAbsolutePositions, 8 ); //variable payload offset
        ++characteristicsCounter;

        //Characteristics count and length in Metadata
        std::vector<std::uint32_t> metadataCharacteristicsLengths( metadataPositions );
        std::transform( metadataCharacteristicsLengths.begin( ), metadataCharacteristicsLengths.end( ),
                        metadataCharacteristicsCountPositions.begin(), metadataCharacteristicsCountPositions.end(),
                        std::minus<std::uint32_t>() );
        MemcpyToBuffers( metadataBuffers, metadataCharacteristicsCountPositions, &characteristicsCounter, 1 );
        MemcpyToBuffers( metadataBuffers, metadataCharacteristicsCountPositions, metadataCharacteristicsLengths, 4 ); //vector to vector
        lf_MovePositions( -5, metadataCharacteristicsCountPositions ); //back to original position

        //here write payload




        ++m_VariablesCount;
    } //end of function


private:

    /**
     * Returns data type index from enum Datatypes
     * @param variable input variable
     * @return data type
     */
    template< class T > inline std::int8_t GetDataType( ) noexcept { return type_unknown; }

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
