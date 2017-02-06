/*
 * BP1.h
 *
 *  Created on: Feb 2, 2017
 *      Author: wfg
 */

#ifndef BP1_H_
#define BP1_H_


namespace adios
{
namespace format
{


struct BP1MetadataSet
{
    std::uint64_t PGCount = 0; ///< number of process groups
    std::uint64_t PGLength = 0; ///< length in bytes of process groups
    std::size_t PGIndexPosition = 16;
    std::vector<char> PGIndex = std::vector<char>( 102400 ); ///< process group index metadata

    std::uint32_t VarsCount = 0; ///< number of written Variables
    std::uint64_t VarsLength = 0; ///< length in bytes of written Variables
    std::size_t   VarsIndexPosition = 12; ///< initial position in bytes
    std::vector<char> VarsIndex = std::vector<char>( 102400 ); ///< metadata variable index, start with 1Kb
//    std::map< std::string, std::pair<std::size_t,std::size_t> > VariablePositions;

    std::uint32_t m_AttributesCount = 0; ///< number of Attributes
    std::uint64_t m_AttributesLength = 0; ///< length in bytes of Attributes
    std::size_t m_AttributesIndexPosition = 12; ///< initial position in bytes
    std::vector<char> m_AttributeIndex = std::vector<char>( 102400 ); ///< metadata attribute index, start with 1Kb

    std::vector<char> m_MiniFooter = std::vector<char>( 28 );
};

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



} //end namespace format
} //end namespace adios



#endif /* BP1_H_ */
