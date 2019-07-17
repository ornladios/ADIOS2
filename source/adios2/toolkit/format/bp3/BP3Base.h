/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Base.h  base class for BP3Serializer and BP3Deserializer
 *
 *  Created on: Feb 2, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_BP3BASE_H_
#define ADIOS2_TOOLKIT_FORMAT_BP3_BP3BASE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <bitset>
#include <map>
#include <memory> //std::shared_ptr
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSConfig.h"
#include "adios2/common/ADIOSMacros.h"
#include "adios2/common/ADIOSTypes.h"
#include "adios2/core/Engine.h"
#include "adios2/core/VariableBase.h"
#include "adios2/toolkit/aggregator/mpi/MPIChain.h"
#include "adios2/toolkit/format/bpOperation/BPOperation.h"
#include "adios2/toolkit/format/buffer/heap/BufferSTL.h"
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

        /** starting point for offsets characteristics update (used in
         * aggregation) */
        size_t LastUpdatedPosition = 0;

        SerialElementIndex(const uint32_t memberID,
                           const size_t bufferSize = 200)
        : MemberID(memberID)
        {
            Buffer.reserve(bufferSize);
        }
    };

    /** Single struct containing metadata indices and tracking information */
    struct MetadataSet
    {
        /**
         * updated with EndStep, if append it will be updated to last,
         * starts with one in ADIOS1 BP3 format
         */
        uint32_t TimeStep = 1;

        /** single buffer for PGIndex */
        SerialElementIndex PGIndex = SerialElementIndex(0);

        // no priority for now
        /** @brief key: variable name, value: bp metadata variable index */
        std::unordered_map<std::string, SerialElementIndex> VarsIndices;

        /** @brief key: attribute name, value: bp metadata attribute index */
        std::unordered_map<std::string, SerialElementIndex> AttributesIndices;

        /** Fixed size for mini footer, adding 28 bytes for ADIOS version */
        const unsigned int MiniFooterSize = 28 + 28;

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

        /** Used at Read, steps start at zero */
        size_t StepsStart = 0;

        /** Used at Read, number of total steps */
        size_t StepsCount = 1;

        /** Similar to TimeStep, but uses uint64_t and start from zero. Used for
         * streaming a large number of steps */
        size_t CurrentStep = 0;
    };

    struct Minifooter
    {
        std::string VersionTag;
        uint64_t PGIndexStart;
        uint64_t VarsIndexStart;
        uint64_t AttributesIndexStart;
        int8_t Version = 3;
        bool IsLittleEndian = true;
        bool HasSubFiles = false;
    };

    MPI_Comm m_MPIComm;  ///< MPI communicator from Engine
    int m_RankMPI = 0;   ///< current MPI rank process
    int m_SizeMPI = 1;   ///< current MPI processes size
    int m_Processes = 1; ///< number of aggregated MPI processes

    /** statistics verbosity, only 0 is supported */
    unsigned int m_StatsLevel = 0;

    /** async threads for background tasks, default = 1, 0 means all serial
     * operations */
    unsigned int m_AsyncThreads = 1;

    /** contains data buffer for this rank */
    BufferSTL m_Data;

    /** contains collective metadata buffer, only used by rank 0 */
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

    /** Parameter to flush transports at every number of steps, to be used at
     * EndStep */
    size_t m_FlushStepsCount = 1;

    /** from host language in data information at read */
    bool m_IsRowMajor = true;

    /** if reader and writer have different ordering (column vs row major) */
    bool m_ReverseDimensions = false;

    /** manages all communication tasks in aggregation */
    aggregator::MPIChain m_Aggregator;

    /** tracks Put and Get variables in deferred mode */
    std::set<std::string> m_DeferredVariables;
    /** tracks the overall size of deferred variables */
    size_t m_DeferredVariablesDataSize = 0;

    /** attributes are serialized only once, this set contains the names of ones
     * already serialized.
     */
    std::unordered_set<std::string> m_SerializedAttributes;

    /**
     * scratch memory buffers used for management of temporary memory buffers
     * per thread.
     * This allows thread-safety mostly is deserialization.
     * Indices:
     * [threadID][bufferID]
     */
    std::map<size_t, std::map<size_t, std::vector<char>>> m_ThreadBuffers;

    /** true: NVMex each rank creates its own directory */
    bool m_NodeLocal = false;

    /**
     * Unique constructor
     * @param mpiComm for m_BP1Aggregator
     * @param debugMode true: exceptions checks
     */
    BP3Base(MPI_Comm mpiComm, const bool debugMode);

    virtual ~BP3Base();

    void InitParameters(const Params &parameters);

    /**
     * Vector version of BPBaseNames
     * @param names
     * @return vector of base (name.bp) names
     */
    std::vector<std::string>
    GetBPBaseNames(const std::vector<std::string> &names) const noexcept;

    /**
     * Get BP substream names from base names:
     * /path/name.bp.dir/name.bp.Index
     * where Index = Rank, and in aggregation = SubStreamID
     * @param baseNames inputs
     * @return vector of BP substream names for transports
     */
    std::vector<std::string>
    GetBPSubStreamNames(const std::vector<std::string> &baseNames) const
        noexcept;

    std::vector<std::string>
    GetBPMetadataFileNames(const std::vector<std::string> &names) const
        noexcept;

    std::string GetBPMetadataFileName(const std::string &name) const noexcept;

    std::string GetBPSubFileName(const std::string &name,
                                 const size_t subFileIndex,
                                 const bool hasSubFiles = true) const noexcept;

    /**
     * Returns the estimated variable index size. Used by ResizeBuffer public
     * function
     * @param variableName input
     * @param count input variable local dimensions
     */
    size_t GetBPIndexSizeInData(const std::string &variableName,
                                const Dims &count) const noexcept;

    /**
     * Sets buffer's positions to zero and fill buffer with zero char
     * @param bufferSTL buffer to be reset
     * @param resetAbsolutePosition true: both bufferSTL.m_Position and
     * bufferSTL.m_AbsolutePosition set to 0,   false(default): only
     * bufferSTL.m_Position
     * is set to zero,
     */
    void ResetBuffer(BufferSTL &bufferSTL,
                     const bool resetAbsolutePosition = false,
                     const bool zeroInitialize = true);

    /** Return type of the CheckAllocation function. */
    enum class ResizeResult
    {
        Failure,   //!< FAILURE, caught a std::bad_alloc
        Unchanged, //!< UNCHANGED, no need to resize (sufficient capacity)
        Success,   //!< SUCCESS, resize was successful
        Flush      //!< FLUSH, need to flush to transports for current variable
    };

    /**
     * Resizes the data buffer to hold new dataIn size
     * @param dataIn input size for new data
     * @param hint for exception handling
     * @return
     * -1: allocation failed,
     *  0: no allocation needed,
     *  1: reallocation is sucessful
     *  2: need a transport flush
     */
    ResizeResult ResizeBuffer(const size_t dataIn, const std::string hint);

    void ProfilerStart(const std::string process) noexcept;

    void ProfilerStop(const std::string process) noexcept;

protected:
    /** might be used in large payload copies to buffer */
    unsigned int m_Threads = 1;
    const bool m_DebugMode = false;

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
        type_string_array = 12,   //!< type_string_array
    };

    /**
     * Maps C++ type to DataTypes enum
     */
    template <typename T>
    struct TypeTraits;

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

    enum TransformTypes
    {
        transform_unknown = -1,
        transform_none = 0,
        transform_identity = 1,
        transform_zlib = 2,
        transform_bzip2 = 3,
        transform_szip = 4,
        transform_isobar = 5,
        transform_aplod = 6,
        transform_alacrity = 7,
        transform_zfp = 8,
        transform_sz = 9,
        transform_lz4 = 10,
        transform_blosc = 11,
        transform_mgard = 12,
        transform_png = 13,
    };

    static const std::set<std::string> m_TransformTypes;
    static const std::map<int, std::string> m_TransformTypesToNames;

    /** Returns the proper derived class for BP3Operation based on type
     * @param type input, must be a supported type under bp3/operation
     * @return derived class if supported, false pointer if type not supported
     */
    std::shared_ptr<BPOperation> SetBPOperation(const std::string type) const
        noexcept;

    template <class T>
    std::map<size_t, std::shared_ptr<BPOperation>> SetBPOperations(
        const std::vector<core::VariableBase::Operation> &operations) const;

    struct ProcessGroupIndex
    {
        uint64_t Offset;
        uint32_t Step;
        int32_t ProcessID;
        uint16_t Length;
        std::string Name;
        std::string StepName;
        char IsColumnMajor;
    };

    /** pre-transform shape */
    struct BP3OpInfo
    {
        std::vector<char> Metadata;
        // pre-operator dimensions
        Dims PreShape;
        Dims PreCount;
        Dims PreStart;
        std::string Type; // Operator type, not data type
        uint8_t PreDataType;
        bool IsActive = false;
    };

    template <class T>
    struct Stats
    {
        double BitSum = 0.;
        double BitSumSquare = 0.;
        uint64_t Offset = 0;
        uint64_t PayloadOffset = 0;
        T Min;
        T Max;
        T Value;
        std::vector<T> Values;
        uint32_t Step = 0;
        uint32_t FileIndex = 0;
        uint32_t MemberID = 0;
        uint32_t BitCount = 0;
        std::bitset<32> Bitmap;
        uint8_t BitFinite = 0;
        bool IsValue = false;
        BP3OpInfo Op;

        Stats() : Min(), Max(), Value() {}
    };

    template <class T>
    struct Characteristics
    {
        Stats<T> Statistics;
        Dims Shape;
        Dims Start;
        Dims Count;
        ShapeID EntryShapeID = ShapeID::Unknown;
        uint32_t EntryLength = 0;
        uint8_t EntryCount = 0;
    };

    struct ElementIndexHeader
    {
        uint64_t CharacteristicsSetsCount;
        uint32_t Length;
        uint32_t MemberID;
        std::string GroupName;
        std::string Name;
        std::string Path;
        uint8_t DataType = std::numeric_limits<uint8_t>::max() - 1;
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

    /** Set available number of threads for vector operations */
    void InitParameterAsyncThreads(const std::string value);

    /** verbose file level=0 (default), not active yet */
    void InitParameterStatLevel(const std::string value);

    /** verbose file level=0 (default) */
    void InitParameterCollectiveMetadata(const std::string value);

    /** set steps count to flush */
    void InitParameterFlushStepsCount(const std::string value);

    /** set number of substreams, turns on aggregation if less < MPI_Size */
    void InitParameterSubStreams(const std::string value);

    /** Sets if IO is node-local so each rank creates its own IO directory and
     * stream */
    void InitParameterNodeLocal(const std::string value);

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

    ProcessGroupIndex ReadProcessGroupIndexHeader(
        const std::vector<char> &buffer, size_t &position,
        const bool isLittleEndian = true) const noexcept;

    ElementIndexHeader
    ReadElementIndexHeader(const std::vector<char> &buffer, size_t &position,
                           const bool isLittleEndian = true) const noexcept;

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
                                    size_t &position, const DataTypes dataType,
                                    const bool untilTimeStep = false,
                                    const bool isLittleEndian = true) const;

    /**
     * Common function to extract a bp string, 2 bytes for length + contents
     * @param buffer
     * @param position
     * @return
     */
    std::string ReadBP3String(const std::vector<char> &buffer, size_t &position,
                              const bool isLittleEndian = true) const noexcept;

    // Transform related functions
    /**
     * Translates string to enum
     * @param transformType input
     * @return corresponding enum TransformTypes
     */
    TransformTypes TransformTypeEnum(const std::string transformType) const
        noexcept;

private:
    std::string GetBPSubStreamName(const std::string &name, const size_t rank,
                                   const bool hasSubFiles = true) const
        noexcept;

    /**
     * Specialized template for string and other types
     */
    template <class T>
    void ParseCharacteristics(const std::vector<char> &buffer, size_t &position,
                              const DataTypes dataType,
                              const bool untilTimeStep,
                              Characteristics<T> &characteristics,
                              const bool isLittleEndian = true) const;
};

#define declare_template_instantiation(T)                                      \
    extern template BP3Base::Characteristics<T>                                \
    BP3Base::ReadElementIndexCharacteristics(                                  \
        const std::vector<char> &, size_t &, const BP3Base::DataTypes,         \
        const bool, const bool) const;                                         \
                                                                               \
    extern template std::map<size_t, std::shared_ptr<BPOperation>>             \
    BP3Base::SetBPOperations<T>(                                               \
        const std::vector<core::VariableBase::Operation> &) const;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#include "BP3Base.inl"

#endif /* ADIOS2_TOOLKIT_FORMAT_BP3_BP3BASE_H_ */
