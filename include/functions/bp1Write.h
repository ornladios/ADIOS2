/*
 * bp1Format.h
 *
 *  Created on: Jan 12, 2017
 *      Author: wfg
 */

#ifndef BP1WRITE_H_
#define BP1WRITE_H_

#include <algorithm> //std::count
#include <cstring> //std::memcpy


#include "core/Group.h"
#include "core/Variable.h"
#include "core/Capsule.h"


namespace adios
{


enum DataTypes {
    type_unknown = -1,
    type_byte = 0, // char type
    type_short = 1,
    type_integer = 2,
    type_long = 4,

    type_unsigned_byte = 50,
    type_unsigned_short = 51,
    type_unsigned_integer = 52,
    type_unsigned_long = 54,

    type_real = 5, // float
    type_double = 6,
    type_long_double = 7,

    type_string = 9,
    type_complex = 10,
    type_double_complex = 11,
    type_string_array = 12
};



/** Define variable characteristic IDs in bp1 format */
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
 * Write a piece of
 * @param buffers
 * @param source
 * @param size
 * @param offset
 */
template< class T >
void WriteToBuffers( std::vector<char*>& buffers, const T* source, std::size_t size, std::size_t& offset )
{
    for( auto& buffer : buffers )
    {
        std::memcpy( &buffer[offset], source, size );
    }
    offset += size;
}


template< class T>
std::int8_t GetBPDataType( const Variable<T>& variable )
{
    int dataType = -1;
    if( std::is_same<T,char> )
        dataType = type_byte;

    else if( std::is_same<T,short> )
        dataType = type_short;

    else if( std::is_same<T,int> )
        dataType = type_integer;

    else if( std::is_same<T,long int> )
        dataType = type_long;

    else if( std::is_same<T,unsigned char> )
        dataType = type_unsigned_byte;

    else if( std::is_same<T,unsigned short> )
        dataType = type_unsigned_short;

    else if( std::is_same<T,unsigned int> )
        dataType = type_unsigned_integer;

    else if( std::is_same<T,unsigned long int> )
        dataType = type_unsigned_long;

    else if( std::is_same<T,float> )
        dataType = type_real;

    else if( std::is_same<T,double> )
        dataType = type_double;

    else if( std::is_same<T,long double> )
        dataType = type_long_double;

    //need to implement complex and string
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
void WriteVariableToBuffers( const Group& group, const std::string variableName,
                             const Variable<T>& variable, std::vector<char*>& dataBuffers,
                             const std::size_t dataBuffersPosition,
                             std::vector<char*>& metadataBuffers,
                             const std::size_t metadataBuffersPosition,
                             const unsigned int memberID, const bool writeDimensionsInData,
                             const unsigned int characteristicsVerbosity )
{
   std::size_t metadataOffset = metadataBuffersPosition + 8; //length of var, will come at the end from this offset
   std::size_t dataOffset = dataBuffersPosition + 8; //length of var, will come at the end from this offset

   //memberID
   WriteToBuffers( metadataBuffers, &memberID, 4, metadataOffset );
   //group
   const std::uint16_t lengthGroupName = group.m_Name.length();
   WriteToBuffers( metadataBuffers, &lengthGroupName, 2, metadataOffset ); //2 bytes
   WriteToBuffers( metadataBuffers, &group.m_Name.c_str(), lengthGroupName, metadataOffset );
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

   //Characteristics Sets in Metadata
   const std::size_t metadataCharacteristicPosition = metadataOffset;
   const std::uint64_t sets = 1; //write one for now
   WriteToBuffers( metadataBuffers, &sets, 8, metadataOffset );

   unsigned int characteristicsCounter = 0; //used for characteristics count, characteristics length will be calculated at the end

   //Get dimensions
   const std::vector<unsigned long long int> localDimensions = group.GetDimensions( variable.DimensionsCSV );

   //write to metadata characteristic
   //characteristic: dimension
   const std::uint8_t characteristicID = characteristic_dimensions;
   WriteToBuffers( metadataBuffers, &characteristicID, 1, metadataOffset );
   const std::uint8_t dimensions = localDimensions.size();
   WriteToBuffers( metadataBuffers, &dimensions, 1, metadataOffset );
   const std::uint16_t dimensionsLength = dimensions * 24; //24 is from 8 bytes for each: local, global dimension, global offset
   WriteToBuffers( metadataBuffers, &dimensions, 2, metadataOffset );

   //dimensions in data buffer
   if( writeDimensionsInData == true )
   {
       const char yes = 'y';
       WriteToBuffers( dataBuffers, &yes, 1, dataOffset );
       //for now only support unsigned long integer value (8 bytes), as in metadata
       WriteToBuffers( dataBuffers, &dimensions, 1, dataOffset );
       const std::uint16_t dimensionsLengthInData = dimensions * 27; //27 is from 9 bytes for each: var y/n + local, var y/n + global dimension, var y/n + global offset
       WriteToBuffers( dataBuffers, &dimensionsLengthInData, 2, dataOffset );
   }
   else
   {
       const char no = 'n';
       WriteToBuffers( dataBuffers, &no, 1, dataOffset );
   }


   if( variable.GlobalBoundsIndex == -1 ) //local variable
   {
       for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
       {
           //metadata
           WriteToBuffers( metadataBuffers, &localDimensions[d], 8, metadataOffset );
           metadataOffset += 16; //skipping global dimension(8), global offset (8)

           //data
           if( writeDimensionsInData == true )
           {
               const char no = 'n';
               WriteToBuffers( dataBuffers, &no, 1, dataOffset );
               WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
               dataOffset += 18; //skipping var no + global dimension(8), var no + global offset (8)
           }
       }
   }
   else //global variable
   {
       std::vector<unsigned long long int> globalDimensions = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].first );
       std::vector<unsigned long long int> globalOffsets = group.GetDimensions( group.m_GlobalBounds[variable.GlobalBoundsIndex].second );

       for( unsigned int d = 0; d < (unsigned int)localDimensions.size(); ++d )
       {
           //metadata
           WriteToBuffers( metadataBuffers, &localDimensions[d], 8, metadataOffset );
           WriteToBuffers( metadataBuffers, &globalDimensions[d], 8, metadataOffset );
           WriteToBuffers( metadataBuffers, &globalOffsets[d], 8, metadataOffset );

           //data
           if( writeDimensionsInData == true )
           {
               const char no = 'n';
               WriteToBuffers( dataBuffers, &no, 1, dataOffset );
               WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );
               WriteToBuffers( dataBuffers, &no, 1, dataOffset );
               WriteToBuffers( dataBuffers, &localDimensions[d], 8, dataOffset );

           }
       }
   }


}




template< class T >
size_t GetVariableIndexSize( const Group& group, const std::string variableName,
                             const Variable<T> variable, const unsigned int characteristicsVerbosity )
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
    if( characteristicsVerbosity == 0 ) //default, only min and max
    {
        indexSize += 2 * ( sizeof(T) + 1 );
        indexSize += 1 + 1; //id
    }

    //need to add transform
}





} //end namespace



#endif /* BP1WRITE_H_ */
