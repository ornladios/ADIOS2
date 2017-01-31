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
#include <algorithm> //std::count, std::copy
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
     * Returns data type index from enum Datatypes
     * @param variable input variable
     * @return data type
     */
    template< class T >
    std::int8_t GetDataType( const Variable<T>& variable ) noexcept
    {
        std::int8_t dataType = -1;
        if( std::is_same<T,char>::value )
            dataType = type_byte;

        else if( std::is_same<T,short>::value )
            dataType = type_short;

        else if( std::is_same<T,int>::value )
            dataType = type_integer;

        else if( std::is_same<T,long int>::value )
            dataType = type_long;

        else if( std::is_same<T,unsigned char>::value )
            dataType = type_unsigned_byte;

        else if( std::is_same<T,unsigned short>::value )
            dataType = type_unsigned_short;

        else if( std::is_same<T,unsigned int>::value )
            dataType = type_unsigned_integer;

        else if( std::is_same<T,unsigned long int>::value )
            dataType = type_unsigned_long;

        else if( std::is_same<T,float>::value )
            dataType = type_real;

        else if( std::is_same<T,double>::value )
            dataType = type_double;

        else if( std::is_same<T,long double>::value )
            dataType = type_long_double;
        //need to implement complex and string
        return dataType;
    }


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
    void WriteToBuffers( std::vector<char*>& buffers, const T* source, std::size_t size, std::size_t& offset ) noexcept
    {
        for( auto& buffer : buffers )
        {
            std::memcpy( &buffer[offset], source, size );
        }
        offset += size;
    }

    template< class T, class U >
    void CopyToBuffers( std::vector<char*>& buffers, const T* source, U size, std::size_t& offset ) noexcept
    {
        for( auto& buffer : buffers )
        {
            std::copy( source, source+size, &buffer[offset] );
        }
        offset += size;
    }

    /**
     * Writes a variable to BP format in data and metadata (index) buffers
     * @param group variable owner
     * @param variableName name of variable to be written
     * @param variable object carrying variable information
     * @param dataBuffers buffers to which variable metadata and payload (values) will be written. Metadata is added in case of system corruption to allow regeneration.
     * @param dataPosition initial data position
     * @param metadataBuffers buffers to which only variable metadata will be written
     * @param metadataPosition
     * @param memberID
     * @return number of bytes written, needed for variable entry length
     */
    template< class T >
    void WriteVariable( const Group& group, const Var variableName,
                        const Variable<T>& variable,
                        std::vector<char*>& dataBuffers, const std::size_t dataPosition,
                        std::vector<char*>& metadataBuffers, const std::size_t metadataPosition ) noexcept
    {
        std::size_t metadataOffset = metadataPosition + 4; //length of var, will come at the end from this offset
        std::size_t dataOffset = dataPosition + 8; //length of var, will come at the end from this offset

        //memberID
        WriteToBuffers( metadataBuffers, &m_VariablesCount, 4, metadataOffset );
        //group, only in metadata
        const std::uint16_t lengthGroupName = group.m_Name.length();
        WriteToBuffers( metadataBuffers, &lengthGroupName, 2, metadataOffset ); //2 bytes
        WriteToBuffers( metadataBuffers, group.m_Name.c_str(), lengthGroupName, metadataOffset );
        //variable name to metadata and data
        const std::uint16_t lengthVariableName = variableName.length();
        WriteToBuffers( metadataBuffers, &lengthVariableName, 2, metadataOffset );
        WriteToBuffers( metadataBuffers, variableName.c_str(), lengthVariableName, metadataOffset );
        WriteToBuffers( dataBuffers, &lengthVariableName, 2, dataOffset );
        WriteToBuffers( dataBuffers, variableName.c_str(), lengthVariableName, dataOffset );
        //skip path (jump 2 bytes, already set to zero)
        metadataOffset += 2;
        dataOffset += 2;
        //dataType
        const std::uint8_t dataType = GetDataType( variable );
        WriteToBuffers( metadataBuffers, &dataType, 1, metadataOffset );
        WriteToBuffers( dataBuffers, &dataType, 1, dataOffset );

        //Characteristics Sets in Metadata and Data
        //const std::size_t metadataCharacteristicsSetsCountPosition = metadataOffset; //very important piece
        std::size_t dataCharacteristicsCountPosition = dataOffset;
        const std::uint64_t sets = 1; //write one for now
        WriteToBuffers( metadataBuffers, &sets, 8, metadataOffset );

        std::uint8_t characteristicsCounter = 0; //used for characteristics count, characteristics length will be calculated at the end

        //DIMENSIONS CHARACTERISTIC
        const std::vector<unsigned long long int> localDimensions = group.GetDimensions( variable.DimensionsCSV );

        //write to metadata characteristic
        //characteristic: dimension
        std::uint8_t characteristicID = characteristic_dimensions;
        WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset );
        const std::uint8_t dimensions = localDimensions.size();
        WriteToBuffers( metadataBuffers, &dimensions, 1, metadataOffset );
        const std::uint16_t dimensionsLength = dimensions * 24; //24 is from 8 bytes for each: local dimension, global dimension, global offset
        WriteToBuffers( metadataBuffers, &dimensionsLength, 2, metadataOffset );

        //write in data if it's a dimension variable (scalar) y or n
        const char dimensionYorN = ( variable.IsDimension ) ? 'y' : 'n';
        WriteToBuffers( dataBuffers, &dimensionYorN, 1, dataOffset );
        WriteToBuffers( dataBuffers, &dimensions, 1, dataOffset );
        const std::uint16_t dimensionsLengthInData = dimensions * 27; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset
        WriteToBuffers( dataBuffers, &dimensionsLengthInData, 2, dataOffset );

        if( variable.GlobalBoundsIndex == -1 ) //local variable
        {
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                //metadata characteristics
                WriteToBuffers( metadataBuffers, &localDimensions[d], 8, metadataOffset );
                metadataOffset += 16; //skipping global dimension(8), global offset (8)
            }

            const char no = 'n'; //dimension format unsigned int value (not using memberID for now)
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                //data dimensions
                WriteToBuffers( dataBuffers, &no, 1, dataOffset );
                WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
                dataOffset += 18; //skipping var no + global dimension(8), var no + global offset (8)
            }

            //dimensions in data characteristic entry
            dataCharacteristicsCountPosition = dataOffset;
            dataOffset += 5; //skip characteristics count(1) + length (4)
            WriteToBuffers( dataBuffers, &characteristicID, 1, dataOffset );

            std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
            WriteToBuffers( dataBuffers, &lengthOfDimensionsCharacteristic, 2, dataOffset );

            WriteToBuffers( dataBuffers, &dimensions, 1, dataOffset );
            WriteToBuffers( dataBuffers, &dimensionsLength, 2, dataOffset );

            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                //data characteristics
                WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
                metadataOffset += 16; //skipping global dimension(8), global offset (8)
            }
        }
        else //global variable
        {
            std::vector<unsigned long long int> globalDimensions = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].first );
            std::vector<unsigned long long int> globalOffsets = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].second );

            //metadata, writing in characteristics
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                WriteToBuffers( metadataBuffers, &localDimensions[d], 8, metadataOffset );
                WriteToBuffers( metadataBuffers, &globalDimensions[d], 8, metadataOffset );
                WriteToBuffers( metadataBuffers, &globalOffsets[d], 8, metadataOffset );
            }

            //data dimensions entry
            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                const char no = 'n'; //dimension format unsigned int value
                WriteToBuffers( dataBuffers, &no, 1, dataOffset );
                WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
                WriteToBuffers( dataBuffers, &no, 1, dataOffset );
                WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
            }

            //dimensions in data characteristic entry (might not be required)
            dataCharacteristicsCountPosition = dataOffset;
            dataOffset += 5; //skip characteristics count(1) + length (4)
            WriteToBuffers( dataBuffers, &characteristicID, 1, dataOffset ); //id

            std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
            WriteToBuffers( dataBuffers, &lengthOfDimensionsCharacteristic, 2, dataOffset );

            WriteToBuffers( dataBuffers, &dimensions, 1, dataOffset );
            WriteToBuffers( dataBuffers, &dimensionsLength, 2, dataOffset );

            for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
            {
                WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
                WriteToBuffers( dataBuffers, &globalDimensions[d], 8, dataOffset );
                WriteToBuffers( dataBuffers, &globalOffsets[d], 8, dataOffset );
            }
        }
        ++characteristicsCounter;

        //VALUE for SCALAR or STAT min, max for ARRAY
        //Value for scalar
        if( variable.DimensionsCSV == "1" ) //scalar //just doing string scalars for now (by name), needs to be modified when user passes value
        {
            characteristicID = characteristic_value;
            std::int16_t lenghtOfName = variableName.length();
            //metadata
            WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset  );
            WriteToBuffers( metadataBuffers, &lenghtOfName, 2, metadataOffset  );
            WriteToBuffers( metadataBuffers, variableName.c_str(), lenghtOfName, metadataOffset  );

            //data
            WriteToBuffers( dataBuffers, &characteristicID, 1, dataOffset  );
            std::int16_t lengthOfCharacteristic = 2 + lenghtOfName;
            WriteToBuffers( dataBuffers, &lengthOfCharacteristic, 2, dataOffset );
            WriteToBuffers( dataBuffers, &lenghtOfName, 2, dataOffset  );
            WriteToBuffers( dataBuffers, variableName.c_str(), lenghtOfName, dataOffset  );
        }
        else // Stat -> Min, Max for arrays,
        {
            if( m_Verbosity == 0 ) //default verbose
            {
                //Get min and max
                const std::size_t valuesSize = GetTotalSize( localDimensions );
                //here we can make decisions for threads based on valuesSize
                T min, max;

                if( valuesSize >= 10000000 ) //ten million? this needs actual results
                    GetMinMax( variable.Values, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
                else
                    GetMinMax( variable.Values, valuesSize, min, max );

                //set characteristic ids for min and max
                characteristicID = characteristic_stat;
                const std::int8_t statisticMinID = statistic_min;
                const std::int8_t statisticMaxID = statistic_max;

                //write min and max to metadata
                WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset  ); //min
                WriteToBuffers( metadataBuffers, &statisticMinID, 1, metadataOffset  );
                WriteToBuffers( metadataBuffers, &min, sizeof(T), metadataOffset );

                WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset  ); //max
                WriteToBuffers( metadataBuffers, &statisticMaxID, 1, metadataOffset  );
                WriteToBuffers( metadataBuffers, &max, sizeof(T), metadataOffset );

                //write min and max to data
                WriteToBuffers( dataBuffers, &characteristicID, 1, dataOffset  ); //min
                const std::int16_t lengthCharacteristic = 1 + sizeof( T );
                WriteToBuffers( dataBuffers, &lengthCharacteristic, 2, dataOffset  );
                WriteToBuffers( dataBuffers, &statisticMinID, 1, dataOffset  );
                WriteToBuffers( dataBuffers, &min, sizeof(T), dataOffset );

                WriteToBuffers( dataBuffers, &characteristicID, 1, dataOffset  ); //max
                WriteToBuffers( dataBuffers, &lengthCharacteristic, 2, dataOffset  );
                WriteToBuffers( dataBuffers, &statisticMaxID, 1, dataOffset  );
                WriteToBuffers( dataBuffers, &max, sizeof(T), dataOffset );
            }
        }
        ++characteristicsCounter;
        //here write characteristics count and length to data
        std::uint32_t characteristicsLengthInData = dataOffset - dataCharacteristicsCountPosition;
        WriteToBuffers( dataBuffers, &characteristicsCounter, 1, dataCharacteristicsCountPosition );
        WriteToBuffers( dataBuffers, &characteristicsLengthInData, 4, dataCharacteristicsCountPosition );
        dataCharacteristicsCountPosition -= 5;

        //Offsets should be last and only written to metadata
        characteristicID = characteristic_offset;
        WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset  ); //variable offset id
        WriteToBuffers( metadataBuffers, &dataPosition, 8, metadataOffset ); //variable offset
        ++characteristicsCounter;

        characteristicID = characteristic_payload_offset;
        WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset  ); //variable payload offset id
        WriteToBuffers( metadataBuffers, &dataOffset, 8, metadataOffset ); //variable offset
        ++characteristicsCounter;

        //here write var entry length in metadata


        ++m_VariablesCount;
    } //end of function

};






} //end namespace format
} //end namespace adios

#endif /* BP1WRITER_H_ */
