/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1.h
 *
 *  Created on: Feb 2, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_UTILITIES_FORMAT_BP1_BP1BASE_H_
#define ADIOS2_UTILITIES_FORMAT_BP1_BP1BASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <memory> //std::shared_ptr
#include <unordered_map>
#include <vector>
/// \endcond

#include "BP1Aggregator.h"
#include "BP1Structs.h"
#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/ADIOS_MPI.h"
#include "adios2/capsule/heap/STLVector.h"
#include "adios2/core/Transport.h"

namespace adios
{
namespace format
{

/**
 * Base class for BP1Writer and BP1Reader format
 */
class BP1Base
{

public:
    /** statistics verbosity, only 0 is supported */
    unsigned int m_Verbosity = 0;

    /** contains data buffer and position */
    capsule::STLVector m_Heap = capsule::STLVector(true);

    /** memory growth factor */
    float m_GrowthFactor = 1.5;

    /** max buffer size */
    size_t m_MaxBufferSize = 0;

    /** contains bp1 format metadata */
    BP1MetadataSet m_MetadataSet;

    /** aggregation tasks */
    BP1Aggregator m_BP1Aggregator;

    /**
     * Unique constructor
     * @param mpiComm for m_BP1Aggregator
     * @param debugMode true: exceptions checks
     */
    BP1Base(MPI_Comm mpiComm, const bool debugMode);

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
                       Transport &file) const;

protected:
    /** might be used in large payload copies to buffer */
    unsigned int m_Threads = 1;

    /** method type for file I/O */
    enum IO_METHOD
    {
        METHOD_UNKNOWN = -2,
        METHOD_NULL = -1,
        METHOD_MPI = 0,
        METHOD_DATATAP = 1 // OBSOLETE
        ,
        METHOD_POSIX = 2,
        METHOD_DATASPACES = 3,
        METHOD_VTK = 4 // non-existent
        ,
        METHOD_POSIX_ASCII = 5 // non-existent
        ,
        METHOD_MPI_CIO = 6 // OBSOLETE
        ,
        METHOD_PHDF5 = 7,
        METHOD_PROVENANCE = 8 // OBSOLETE
        ,
        METHOD_MPI_STRIPE = 9 // OBSOLETE
        ,
        METHOD_MPI_LUSTRE = 10,
        METHOD_MPI_STAGGER = 11 // OBSOLETE
        ,
        METHOD_MPI_AGG = 12 // OBSOLETE
        ,
        METHOD_ADAPTIVE = 13 // OBSOLETE
        ,
        METHOD_POSIX1 = 14 // OBSOLETE
        ,
        METHOD_NC4 = 15,
        METHOD_MPI_AMR = 16,
        METHOD_MPI_AMR1 = 17 // OBSOLETE
        ,
        METHOD_FLEXPATH = 18,
        METHOD_NSSI_STAGING = 19,
        METHOD_NSSI_FILTER = 20,
        METHOD_DIMES = 21,
        METHOD_VAR_MERGE = 22,
        METHOD_MPI_BGQ = 23,
        METHOD_ICEE = 24,
        METHOD_COUNT = 25,
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

    template <class T>
    struct Stats
    {
        T Min;
        T Max;
        uint64_t Offset;
        uint64_t PayloadOffset;
        uint32_t TimeIndex;
        uint32_t MemberID;

        //      unsigned long int count;
        //      long double sum;
        //      long double sumSquare;
        // unsigned long int histogram
        // bool finite??
    };

    /**
     * Returns data type index from enum Datatypes
     * @param variable input variable
     * @return data type
     */
    template <class T>
    inline int8_t GetDataType() const noexcept
    {
        return type_unknown;
    }

    std::vector<uint8_t> GetMethodIDs(
        const std::vector<std::shared_ptr<Transport>> &transports) const
        noexcept;
};

// Moving template BP1Writer::GetDataType template specializations outside of
// the class
template <>
inline int8_t BP1Base::GetDataType<char>() const noexcept
{
    return type_byte;
}

template <>
inline int8_t BP1Base::GetDataType<short>() const noexcept
{
    return type_short;
}

template <>
inline int8_t BP1Base::GetDataType<int>() const noexcept
{
    return type_integer;
}
template <>
inline int8_t BP1Base::GetDataType<long int>() const noexcept
{
    return type_long;
}

template <>
inline int8_t BP1Base::GetDataType<unsigned char>() const noexcept
{
    return type_unsigned_byte;
}
template <>
inline int8_t BP1Base::GetDataType<unsigned short>() const noexcept
{
    return type_unsigned_short;
}
template <>
inline int8_t BP1Base::GetDataType<unsigned int>() const noexcept
{
    return type_unsigned_integer;
}
template <>
inline int8_t BP1Base::GetDataType<unsigned long int>() const noexcept
{
    return type_unsigned_long;
}

template <>
inline int8_t BP1Base::GetDataType<float>() const noexcept
{
    return type_real;
}

template <>
inline int8_t BP1Base::GetDataType<double>() const noexcept
{
    return type_double;
}

template <>
inline int8_t BP1Base::GetDataType<long double>() const noexcept
{
    return type_long_double;
}

} // end namespace format
} // end namespace adios

#endif /* ADIOS2_UTILITIES_FORMAT_BP1_BP1BASE_H_ */
