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
#include <algorithm> //std::count
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

    std::vector<char> m_PGIndex; ///< process group index
    std::vector<char> m_VariableIndex; ///< metadata variable index
    std::vector<char> m_AttributeIndex; ///< metadata attribute index

    const unsigned int m_Cores = 1;

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
                                 const Variable<T> variable, const unsigned int verbosity ) noexcept
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
        if( verbosity == 0 ) //default, only min and max
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

    template< class T >
    void CopyToBuffers( std::vector<char*>& buffers, const T* source, std::size_t size, std::size_t& offset ) noexcept
    {
        for( auto& buffer : buffers )
        {
            std::copy( source, source+size, &buffer[offset] );
            //std::memcpy( &buffer[offset], source, size );
        }
        offset += size;
    }



    /**
     *
     * @param group variable owner
     * @param variableName name of variable to be written
     * @param variable object carrying variable information
     * @param dataBuffers buffers to which variable metadata and payload (values) will be written. Metadata is added in case of system corruption to allow regeneration.
     * @param metadataBuffers buffers to which only variable metadata will be written
     * @param characteristicsVerbosity default = 0 min and max only,
     */
    template< class T >
    void WriteVariable( const Group& group, const std::string variableName,
                        const Variable<T>& variable,
                        std::vector<char*>& dataBuffers, const std::size_t dataPosition,
                        std::vector<char*>& metadataBuffers, const std::size_t metadataPosition,
                        const unsigned int memberID, const bool writeDimensionsInData,
                        const unsigned int verbose ) noexcept
    {
       std::size_t metadataOffset = metadataPosition + 8; //length of var, will come at the end from this offset
       std::size_t dataOffset = dataPosition + 8; //length of var, will come at the end from this offset

       //memberID
       WriteToBuffers( metadataBuffers, &memberID, 4, metadataOffset );
       //group
       const std::uint16_t lengthGroupName = group.m_Name.length();
       WriteToBuffers( metadataBuffers, &lengthGroupName, 2, metadataOffset ); //2 bytes
       //const char* groupName = ;
       WriteToBuffers( metadataBuffers, group.m_Name.c_str(), lengthGroupName, metadataOffset );
       //variable name to metadata and data
       const std::uint16_t lengthVariableName = variableName.length();
       WriteToBuffers( metadataBuffers, &lengthVariableName, 2, metadataOffset );
       WriteToBuffers( metadataBuffers, variableName.c_str(), lengthVariableName, metadataOffset );
       WriteToBuffers( dataBuffers, &lengthVariableName, 2, dataOffset );
       WriteToBuffers( dataBuffers, variableName.c_str(), lengthVariableName, dataOffset );
       //skip path (jump 2 bytes)
       metadataOffset += 2;
       dataOffset += 2;
       //dataType
       const std::uint8_t dataType = GetBPDataType( variable );
       WriteToBuffers( metadataBuffers, &dataType, 1, metadataOffset );
       WriteToBuffers( dataBuffers, &dataType, 1, dataOffset );

       //Characteristics Sets in Metadata and Data
       const std::size_t metadataCharacteristicsSetsCountPosition = metadataOffset; //very important piece
       std::size_t dataCharacteristicsCountPosition = dataOffset;
       const std::uint64_t sets = 1; //write one for now
       WriteToBuffers( metadataBuffers, &sets, 8, metadataOffset );

       unsigned int characteristicsCounter = 0; //used for characteristics count, characteristics length will be calculated at the end

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

       //dimensions in data buffer, header
       if( writeDimensionsInData == true )
       {
           const char yes = 'y'; // dimension y or n
           WriteToBuffers( dataBuffers, &yes, 1, dataOffset );
           //for now only support unsigned long integer value (8 bytes), as in metadata
           WriteToBuffers( dataBuffers, &dimensions, 1, dataOffset );
           const std::uint16_t dimensionsLengthInData = dimensions * 27; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset
           WriteToBuffers( dataBuffers, &dimensionsLengthInData, 2, dataOffset );
       }
       else
       {
           const char no = 'n'; // dimension y or n
           WriteToBuffers( dataBuffers, &no, 1, dataOffset );
           dataCharacteristicsCountPosition += 1;
       }

       if( variable.GlobalBoundsIndex == -1 ) //local variable
       {
           for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
           {
               //metadata characteristics
               WriteToBuffers( metadataBuffers, &localDimensions[d], 8, metadataOffset );
               metadataOffset += 16; //skipping global dimension(8), global offset (8)
           }

           if( writeDimensionsInData == true )
           {
               for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
               {
                   const char no = 'n'; //dimension format unsigned int value
                   WriteToBuffers( dataBuffers, &no, 1, dataOffset );
                   WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
                   dataOffset += 18; //skipping var no + global dimension(8), var no + global offset (8)
               }
           }
           //dimensions in data characteristic entry (might not be required)
           dataCharacteristicsCountPosition = dataOffset;
           dataOffset += 5; //skip characteristics count(1) + length (4)
           WriteToBuffers( dataBuffers, &characteristicID, 1, dataOffset );

           std::int16_t lengthOfDimensionsCharacteristic = 3 + 24 * dimensions; // 3 = dimension(1) + length(2) ; 24 = 3 local, global, global offset x 8 bytes/each
           WriteToBuffers( dataBuffers, &lengthOfDimensionsCharacteristic, 2, dataOffset );

           WriteToBuffers( dataBuffers, &dimensions, 1, dataOffset );
           WriteToBuffers( dataBuffers, &dimensionsLength, 2, dataOffset );

           for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
           {
               //metadata characteristics
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
           if( writeDimensionsInData == true )
           {
               for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
               {
                   const char no = 'n'; //dimension format unsigned int value
                   WriteToBuffers( dataBuffers, &no, 1, dataOffset );
                   WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
                   WriteToBuffers( dataBuffers, &no, 1, dataOffset );
                   WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
               }
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
       //Stat -> Min, Max
       characteristicID = characteristic_stat;
       if( verbose == 0 ) //default verbose
       {
           const std::size_t valuesSize = GetTotalSize( localDimensions );
           //here we can make decisions for threads based on valuesSize
           T min, max;

           if( valuesSize >= 10000000 ) //ten million? this needs actual results
               GetMinMax( variable.Values, valuesSize, min, max, m_Cores ); //here we can add cores from constructor
           else
               GetMinMax( variable.Values, valuesSize, min, max );

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
       //Offsets should be last and only written to metadata

    } //end of function


private:

    bool m_DebugMode = false;

};






} //end namespace format
} //end namespace adios

#endif /* BP1WRITER_H_ */
