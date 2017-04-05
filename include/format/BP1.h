/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1.h
 *
 *  Created on: Feb 2, 2017
 *      Author: wfg
 */

#ifndef BP1_H_
#define BP1_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <cstdint> //std::uintX_t
#include <memory>  //std::shared_ptr
#include <unordered_map>
#include <vector>
//#include <queue>  //std::priority_queue to be added later
/// \endcond

#include "ADIOS_MPI.h"

#include "core/Profiler.h"
#include "core/Transport.h"

namespace adios
{
namespace format
{

/**
 * Used for Variables and Attributes, needed in a container for characteristic
 * sets merge independently for each Variable or Attribute
 */
struct BP1Index
{
    std::vector<char> Buffer; ///< metadata variable index, start with 100Kb
    std::uint64_t Count =
        0; ///< number of characteristics sets (time and spatial aggregation)
    const std::uint32_t MemberID;

    BP1Index(const std::uint32_t memberID) : MemberID{memberID}
    {
        Buffer.reserve(500);
    }
};

/**
 * Single struct that tracks metadata indices in bp format
 */
struct BP1MetadataSet
{
    std::uint32_t
        TimeStep; ///< current time step, updated with advance step, if
                  /// append it will be updated to last, starts with one
    /// in ADIOS1

    BP1Index PGIndex = BP1Index(0); ///< single buffer for PGIndex

    // no priority for now
    std::unordered_map<std::string, BP1Index>
        VarsIndices; ///< key: variable name, value: bp metadata variable index
    std::unordered_map<std::string, BP1Index> AttributesIndices; ///< key:
                                                                 /// attribute
    /// name, value:
    /// bp metadata
    /// attribute
    /// index

    const unsigned int MiniFooterSize = 28; ///< from bpls reader

    // PG (relative) positions in Data buffer to be updated
    std::uint64_t DataPGCount = 0;
    std::size_t DataPGLengthPosition = 0; ///< current PG initial ( relative )
                                          /// position, needs to be updated in
    /// every advance step or init
    std::uint32_t DataPGVarsCount = 0; ///< variables in current PG
    std::size_t DataPGVarsCountPosition =
        0; ///< current PG variable count ( relative ) position, needs to be
           /// updated in every advance step or init
    bool DataPGIsOpen = false;

    Profiler Log; ///< object that takes buffering profiling info
};

/**
 * Base class for BP1Writer and BP1Reader format
 */
class BP1
{

public:
    /**
     * Checks if input name has .bp extension and returns a .bp directory name
     * @param name input (might or not have .bp)
     * @return either name.bp (name has no .bp) or name (name has .bp extension)
     */
    std::string GetDirectoryName(const std::string name) const noexcept;

    /**
     * Opens rank files in the following format:
     * if transport.m_MPIComm different from MPI_Comm_SELF -->
     * name.bp.dir/name.bp.rank
     * @param name might contain .bp or not, if not .bp will be added
     * @param accessMode "write" "w", "r" "read",  "append" "a"
     * @param transport file I/O transport
     */
    void OpenRankFiles(const std::string name, const std::string accessMode,
                       Transport &transport) const;

protected:
    /**
     * method type for file I/O
     */
    enum IO_METHOD
    {
        METHOD_UNKNOWN = -2 //!< ADIOS_METHOD_UNKNOWN
        ,
        METHOD_NULL = -1 //!< ADIOS_METHOD_NULL
        ,
        METHOD_MPI = 0 //!< METHOD_MPI
        ,
        METHOD_DATATAP = 1 // OBSOLETE
        ,
        METHOD_POSIX = 2 //!< METHOD_POSIX
        ,
        METHOD_DATASPACES = 3 //!< METHOD_DATASPACES
        ,
        METHOD_VTK = 4 // non-existent
        ,
        METHOD_POSIX_ASCII = 5 // non-existent
        ,
        METHOD_MPI_CIO = 6 // OBSOLETE
        ,
        METHOD_PHDF5 = 7 //!< METHOD_PHDF5
        ,
        METHOD_PROVENANCE = 8 // OBSOLETE
        ,
        METHOD_MPI_STRIPE = 9 // OBSOLETE
        ,
        METHOD_MPI_LUSTRE = 10 //!< METHOD_MPI_LUSTRE
        ,
        METHOD_MPI_STAGGER = 11 // OBSOLETE
        ,
        METHOD_MPI_AGG = 12 // OBSOLETE
        ,
        METHOD_ADAPTIVE = 13 // OBSOLETE
        ,
        METHOD_POSIX1 = 14 // OBSOLETE
        ,
        METHOD_NC4 = 15 //!< METHOD_NC4
        ,
        METHOD_MPI_AMR = 16 //!< METHOD_MPI_AMR
        ,
        METHOD_MPI_AMR1 = 17 // OBSOLETE
        ,
        METHOD_FLEXPATH = 18 //!< METHOD_FLEXPATH
        ,
        METHOD_NSSI_STAGING = 19 //!< METHOD_NSSI_STAGING
        ,
        METHOD_NSSI_FILTER = 20 //!< METHOD_NSSI_FILTER
        ,
        METHOD_DIMES = 21 //!< METHOD_DIMES
        ,
        METHOD_VAR_MERGE = 22 //!< METHOD_VAR_MERGE
        ,
        METHOD_MPI_BGQ = 23 //!< METHOD_MPI_BGQ
        ,
        METHOD_ICEE = 24 //!< METHOD_ICEE
        ,
        METHOD_COUNT = 25 //!< METHOD_COUNT
        ,
        METHOD_FSTREAM = 26,
        METHOD_FILE = 27,
        METHOD_ZMQ = 28,
        METHOD_MDTM = 29
    };

    /**
     * DataTypes mapping in BP Format
     */
    enum DataTypes
    {
        type_unknown = -1, //!< type_unknown
        type_byte = 0,     //!< type_byte
        type_short = 1,    //!< type_short
        type_integer = 2,  //!< type_integer
        type_long = 4,     //!< type_long

        type_unsigned_byte = 50,    //!< type_unsigned_byte
        type_unsigned_short = 51,   //!< type_unsigned_short
        type_unsigned_integer = 52, //!< type_unsigned_integer
        type_unsigned_long = 54,    //!< type_unsigned_long

        type_real = 5,        //!< type_real or float
        type_double = 6,      //!< type_double
        type_long_double = 7, //!< type_long_double

        type_string = 9,          //!< type_string
        type_complex = 10,        //!< type_complex
        type_double_complex = 11, //!< type_double_complex
        type_string_array = 12    //!< type_string_array
    };

    /**
     * Characteristic ID in variable metadata
     */
    enum VariableCharacteristicID
    {
        characteristic_value = 0,      //!< characteristic_value
        characteristic_min = 1,        //!< Used to read in older bp file format
        characteristic_max = 2,        //!< Used to read in older bp file format
        characteristic_offset = 3,     //!< characteristic_offset
        characteristic_dimensions = 4, //!< characteristic_dimensions
        characteristic_var_id = 5,     //!< characteristic_var_id
        characteristic_payload_offset = 6, //!< characteristic_payload_offset
        characteristic_file_index = 7,     //!< characteristic_file_index
        characteristic_time_index = 8,     //!< characteristic_time_index
        characteristic_bitmap = 9,         //!< characteristic_bitmap
        characteristic_stat = 10,          //!< characteristic_stat
        characteristic_transform_type = 11 //!< characteristic_transform_type
    };

    /** Define statistics type for characteristic ID = 10 in bp1 format */
    enum VariableStatistics
    {
        statistic_min = 0,
        statistic_max = 1,
        statistic_cnt = 2,
        statistic_sum = 3,
        statistic_sum_square = 4,
        statistic_hist = 5,
        statistic_finite = 6
    };

    template <class T> struct Stats
    {
        T Min;
        T Max;
        std::uint64_t Offset;
        std::uint64_t PayloadOffset;
        std::uint32_t TimeIndex;
        std::uint32_t MemberID;

        //		unsigned long int count;
        //		long double sum;
        //		long double sumSquare;
        // unsigned long int histogram
        // bool finite??
    };

    /**
     * Returns data type index from enum Datatypes
     * @param variable input variable
     * @return data type
     */
    template <class T> inline std::int8_t GetDataType() const noexcept
    {
        return type_unknown;
    }

    std::vector<std::uint8_t> GetMethodIDs(
        const std::vector<std::shared_ptr<Transport>> &transports) const
        noexcept;
};

// Moving template BP1Writer::GetDataType template specializations outside of
// the class
template <> inline std::int8_t BP1::GetDataType<char>() const noexcept
{
    return type_byte;
}
template <> inline std::int8_t BP1::GetDataType<short>() const noexcept
{
    return type_short;
}
template <> inline std::int8_t BP1::GetDataType<int>() const noexcept
{
    return type_integer;
}
template <> inline std::int8_t BP1::GetDataType<long int>() const noexcept
{
    return type_long;
}

template <> inline std::int8_t BP1::GetDataType<unsigned char>() const noexcept
{
    return type_unsigned_byte;
}
template <> inline std::int8_t BP1::GetDataType<unsigned short>() const noexcept
{
    return type_unsigned_short;
}
template <> inline std::int8_t BP1::GetDataType<unsigned int>() const noexcept
{
    return type_unsigned_integer;
}
template <>
inline std::int8_t BP1::GetDataType<unsigned long int>() const noexcept
{
    return type_unsigned_long;
}

template <> inline std::int8_t BP1::GetDataType<float>() const noexcept
{
    return type_real;
}
template <> inline std::int8_t BP1::GetDataType<double>() const noexcept
{
    return type_double;
}
template <> inline std::int8_t BP1::GetDataType<long double>() const noexcept
{
    return type_long_double;
}

} // end namespace format
} // end namespace adios

#endif /* BP1_H_ */
