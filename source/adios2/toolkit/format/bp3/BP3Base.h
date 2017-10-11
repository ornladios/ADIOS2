/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Base.h  base class for BP1Writer and BP1Reader
 *
 *  Created on: Feb 2, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_H_
#define ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <unordered_map>
#include <vector>
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMPICommOnly.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Variable.h"
#include "adios2/toolkit/format/BufferSTL.h"
#include "adios2/toolkit/profiling/iochrono/IOChrono.h"

namespace adios2
{
namespace format
{

/**
 * Base class for BP1Writer and BP1Reader format
 */
class BP3Base
{

public:
    /**
     * Metadata index used for Variables and Attributes, needed in a
     * container for characteristic sets merge independently for each Variable
     * or Attribute
     */
    struct SerialElementIndex
    {
        /** buffer containing the metadata index, start with 500bytes */
        std::vector<char> Buffer;
        /** number of characteristics sets (time and spatial aggregation) */
        uint64_t Count = 0;
        /** unique ID assigned to each variable for counter */
        const uint32_t MemberID;

        SerialElementIndex(const uint32_t memberID,
                           const size_t bufferSize = 200)
        : MemberID(memberID)
        {
            Buffer.reserve(bufferSize);
        }
    };

    struct MetadataSet
    {
        /**
         * updated with advance step, if append it will be updated to last,
         * starts with one in ADIOS1
         */
        uint32_t TimeStep = 1;

        /** single buffer for PGIndex */
        SerialElementIndex PGIndex = SerialElementIndex(0);

        // no priority for now
        /** @brief key: variable name, value: bp metadata variable index */
        std::unordered_map<std::string, SerialElementIndex> VarsIndices;

        /** @brief key: attribute name, value: bp metadata attribute index */
        std::unordered_map<std::string, SerialElementIndex> AttributesIndices;

        bool AreAttributesWritten = false;

        /** Fixed size for mini footer */
        const unsigned int MiniFooterSize = 28;

        /** number of current PGs */
        uint64_t DataPGCount = 0;
        /** current PG initial ( relative ) position in data buffer */
        size_t DataPGLengthPosition = 0;
        /** number of variables in current PG */
        uint32_t DataPGVarsCount = 0;
        /** current PG variable count ( relative ) position */
        size_t DataPGVarsCountPosition = 0;
        /** true: currently writing to a pg, false: no current pg */
        bool DataPGIsOpen = false;
    };

    struct Minifooter
    {
        uint64_t PGIndexStart;
        uint64_t VarsIndexStart;
        uint64_t AttributesIndexStart;
        uint8_t Version = 3;
        bool IsLittleEndian = true;
        bool HasSubFiles = false;
    };

    MPI_Comm m_MPIComm;  ///< MPI communicator from Engine
    int m_RankMPI = 0;   ///< current MPI rank process
    int m_SizeMPI = 1;   ///< current MPI processes size
    int m_Processes = 1; ///< number of aggregated MPI processes

    /** statistics verbosity, only 0 is supported */
    unsigned int m_Verbosity = 0;

    /** contains data buffer and position */
    // capsule::STLVector m_HeapBuffer;
    BufferSTL m_Data;
    BufferSTL m_Metadata;

    /** memory growth factor,s set by the user */
    float m_GrowthFactor = DefaultBufferGrowthFactor;

    /** max buffer size, set by the user */
    size_t m_MaxBufferSize = DefaultMaxBufferSize;

    /** contains bp1 format metadata indices*/
    MetadataSet m_MetadataSet;

    /** true: Close was called, Engine will call this many times for different
     * transports */
    bool m_IsClosed = false;

    /** buffering and MPI aggregation profiling info, set by user */
    profiling::IOChrono m_Profiler;

    /** Default: write collective metadata in Capsule metadata. */
    bool m_CollectiveMetadata = true;

    /**
     * Unique constructor
     * @param mpiComm for m_BP1Aggregator
     * @param debugMode true: exceptions checks
     */
    BP3Base(MPI_Comm mpiComm, const bool debugMode);

    virtual ~BP3Base() = default;

    void InitParameters(const Params &parameters);

    /**
     * Vector version of BPBaseNames
     * @param names
     * @return vector of base (name.bp) names
     */
    std::vector<std::string>
    GetBPBaseNames(const std::vector<std::string> &names) const noexcept;

    /**
     * Get BP names from base names
     * @param names inputs
     * @return
     */
    std::vector<std::string>
    GetBPNames(const std::vector<std::string> &baseNames) const noexcept;

    std::string GetBPMetadataFileName(const std::string &name) const noexcept;

    /** Return type of the CheckAllocation function. */
    enum class ResizeResult
    {
        Failure,   //!< FAILURE, caught a std::bad_alloc
        Unchanged, //!< UNCHANGED, no need to resize (sufficient capacity)
        Success,   //!< SUCCESS, resize was successful
        Flush      //!< FLUSH, need to flush to transports for current variable
    };

    /**
     * @param variable
     * @return
     * -1: allocation failed,
     *  0: no allocation needed,
     *  1: reallocation is sucessful
     *  2: need a transport flush
     */
    template <class T>
    ResizeResult ResizeBuffer(const Variable<T> &variable);

protected:
    /** might be used in large payload copies to buffer */
    unsigned int m_Threads = 1;
    const bool m_DebugMode = false;

    /** from host language */
    bool m_IsRowMajor = true; // C, C++ defaults
    /** from host language */
    bool m_IsZeroIndex = true; // C, C++

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

        type_string = 9,              //!< type_string
        type_complex = 10,            //!< type_complex
        type_double_complex = 11,     //!< type_double_complex
        type_string_array = 12,       //!< type_string_array
        type_long_double_complex = 13 //!< type_double_complex
    };

    /**
     * Characteristic ID in variable metadata
     */
    enum CharacteristicID
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
        uint64_t Offset;
        uint64_t PayloadOffset;
        T Min;
        T Max;
        uint32_t Step;
        uint32_t FileIndex;
        uint32_t MemberID;
    };

    template <class T>
    struct Characteristics
    {
        Stats<T> Statistics;
        Dims Shape;
        Dims Start;
        Dims Count;
        uint32_t EntryLength;
        uint8_t EntryCount;
    };

    struct ElementIndexHeader
    {
        uint64_t CharacteristicsSetsCount;
        uint32_t Length;
        uint32_t MemberID;
        std::string GroupName;
        std::string Name;
        std::string Path;
        uint8_t DataType;
    };

    /**
     * Functions used for setting bool parameters of type On Off
     * @param value
     * @param parameter
     * @param hint
     */
    void InitOnOffParameter(const std::string value, bool &parameter,
                            const std::string hint);

    /** profile=on (default) generate profiling.log
     *  profile=off */
    void InitParameterProfile(const std::string value);

    /** profile_units=s (default) (mus, ms, s,m,h) from ADIOSTypes.h TimeUnit */
    void InitParameterProfileUnits(const std::string value);

    /** growth_factor=1.5 (default), must be > 1.0 */
    void InitParameterBufferGrowth(const std::string value);

    /** set initial buffer size */
    void InitParameterInitBufferSize(const std::string value);

    /** default = DefaultMaxBufferSize in ADIOSTypes.h, set max buffer size in
     * Gb or Mb
     *  max_buffer_size=100Mb or  max_buffer_size=1Gb */
    void InitParameterMaxBufferSize(const std::string value);

    /** Set available number of threads for vector operations */
    void InitParameterThreads(const std::string value);

    /** verbose file level=0 (default), not active */
    void InitParameterVerbose(const std::string value);

    /** verbose file level=0 (default) */
    void InitParameterCollectiveMetadata(const std::string value);

    /**
     * Returns data type index from enum Datatypes
     * @param variable input variable
     * @return data type
     */
    template <class T>
    int8_t GetDataType() const noexcept;

    std::vector<uint8_t>
    GetTransportIDs(const std::vector<std::string> &transportsTypes) const
        noexcept;

    /**
     * Calculates the Process Index size in bytes according to the BP
     * format,
     * including list of method with no parameters (for now)
     * @param name
     * @param timeStepName
     * @param transportsSize
     * @return size of pg index
     */
    size_t GetProcessGroupIndexSize(const std::string name,
                                    const std::string timeStepName,
                                    const size_t transportsSize) const noexcept;

    ElementIndexHeader ReadElementIndexHeader(const std::vector<char> &buffer,
                                              size_t &position) const noexcept;

    /**
     * Read variable characteristics.
     * @param buffer
     * @param position
     * @param untilTimeStep, stop if time step characteristic is found
     * @return
     */
    template <class T>
    Characteristics<T>
    ReadElementIndexCharacteristics(const std::vector<char> &buffer,
                                    size_t &position,
                                    const bool untilTimeStep = false) const;

    /**
     * Common function to extract a bp string, 2 bytes for length + contents
     * @param buffer
     * @param position
     * @return
     */
    std::string ReadBP1String(const std::vector<char> &buffer,
                              size_t &position) const noexcept;

    void ProfilerStart(const std::string process);

    void ProfilerStop(const std::string process);

private:
    /**
     * Returns the estimated variable index size. Used by ResizeBuffer public
     * function
     * @param variable input
     */
    template <class T>
    size_t GetVariableIndexSize(const Variable<T> &variable) const noexcept;
};

#define declare_template_instantiation(T)                                      \
    extern template BP3Base::ResizeResult BP3Base::ResizeBuffer(               \
        const Variable<T> &variable);                                          \
                                                                               \
    extern template BP3Base::Characteristics<T>                                \
    BP3Base::ReadElementIndexCharacteristics(const std::vector<char> &buffer,  \
                                             size_t &position,                 \
                                             const bool untilTimeStep) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP1_BP1BASE_H_ */
