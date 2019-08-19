/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Serializer.h
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_BP4SERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP4_BP4SERIALIZER_H_

#include <mutex>

#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/format/bp4/BP4Base.h"

namespace adios2
{
namespace format
{

class BP4Serializer : public BP4Base
{

public:
    /**
     * Unique constructor
     * @param mpiComm MPI communicator for BP1 Aggregator
     * @param debug true: extra checks
     */
    BP4Serializer(MPI_Comm mpiComm, const bool debugMode = false);

    ~BP4Serializer() = default;

    /** Writes a 64 byte header into the data/metadata buffer.
     *  Must be called only when the buffer is empty.
     *  @param buffer the data or metadata buffer
     *  @param fileType a small string up to 8 characters that is
     *  concatenated to the version string
     */
    void MakeHeader(BufferSTL &b, const std::string fileType,
                    const bool isActive);

    /**
     * Writes a process group index PGIndex and list of methods (from
     * transports). Done at Open or Advance.
     * @param ioName from IO class, identify Process Group with IO name
     * @param hostLanguage from ADIOS class passed to IO
     * @param transportsTypes passed to get list of transport "bp methods"
     */
    void PutProcessGroupIndex(
        const std::string &ioName, const std::string hostLanguage,
        const std::vector<std::string> &transportsTypes) noexcept;

    /**
     * Put in buffer metadata for a given variable
     * @param variable
     */
    template <class T>
    void PutVariableMetadata(const core::Variable<T> &variable,
                             const typename core::Variable<T>::Info &blockInfo,
                             const bool sourceRowMajor = true) noexcept;

    /**
     * Put in buffer variable payload. Expensive part.
     * @param variable payload input from m_PutValues
     */
    template <class T>
    void PutVariablePayload(const core::Variable<T> &variable,
                            const typename core::Variable<T>::Info &blockInfo,
                            const bool sourceRowMajor = true) noexcept;

    /**
     *  Serializes data buffer and close current process group
     * @param io : attributes written in first step
     * @param advanceStep true: advances step, false: doesn't advance
     */
    void SerializeData(core::IO &io, const bool advanceStep = false);

    /**
     * Serializes the metadata indices appending it into the data buffer inside
     * m_Data
     * @param updateAbsolutePosition true: adds footer size to absolute position
     * used for Close,
     * false: doesn't update absolute, used for partial buffer
     * @param inData true: serialize in data , false: only update metadata
     * indices, do not serialize in data
     */
    void SerializeMetadataInData(const bool updateAbsolutePosition = true,
                                 const bool inData = true);

    /**
     * Finishes bp buffer by serializing data and adding trailing metadata
     * @param io
     * @return the position of the data buffer before writing the metadata
     * footer into it (in case someone wants to write only the data portion)
     */
    size_t CloseData(core::IO &io);

    /**
     * Closes bp buffer for streaming mode...must reset metadata for the next
     * step
     * @param io object containing all attributes
     * @param addMetadata true: process metadata and add to data buffer
     * (minifooter)
     * @return the position of the data buffer before writing the metadata
     * footer into it (in case someone wants to write only the data portion)
     */
    size_t CloseStream(core::IO &io, const bool addMetadata = true);
    size_t CloseStream(core::IO &io, size_t &metadataStart,
                       size_t &metadataCount, const bool addMetadata = true);

    void ResetIndices();

    /* Reset all metadata indices at the end of each step */
    void ResetAllIndices();

    /* Reset metadata index table*/
    void ResetMetadataIndexTable();

    /**
     * Get a string with profiling information for this rank
     * @param name stream name
     * @param transportsTypes list of transport types
     * @param transportsProfilers list of references to transport profilers
     */
    std::string GetRankProfilingJSON(
        const std::vector<std::string> &transportsTypes,
        const std::vector<profiling::IOChrono *> &transportsProfilers) noexcept;

    /**
     * Forms the final profiling.json string aggregating from all ranks
     * @param rankProfilingJSON
     * @return profiling.json
     */
    std::vector<char>
    AggregateProfilingJSON(const std::string &rankProfilingJSON);

    /**
     * Aggregate collective metadata
     * @param comm input establishing domain (all or per aggregator)
     * @param bufferSTL buffer to put the metadata
     * @param inMetadataBuffer collective metadata from absolute rank = 0, else
     *                         from aggregators
     */
    void AggregateCollectiveMetadata(MPI_Comm comm, BufferSTL &bufferSTL,
                                     const bool inMetadataBuffer);

    /**
     * Updates variable and payload offsets in metadata characteristics with
     * the updated Buffer m_DataAbsolutePosition for a particular rank. This is
     * a local (non-MPI) operation
     */
    void UpdateOffsetsInMetadata();

private:
    std::vector<char> m_SerializedIndices;
    std::vector<char> m_GatheredSerializedIndices;

    /** BP format version */
    const uint8_t m_Version = 4;

    static std::mutex m_Mutex;

    /** aggregate pg rank indices */
    std::unordered_map<size_t, std::vector<std::tuple<size_t, size_t, size_t>>>
        m_PGIndicesInfo;
    /** deserialized variable indices per rank (vector index) */
    std::unordered_map<
        size_t, std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>>
        m_VariableIndicesInfo;
    /** deserialized attribute indices per rank (vector index) */
    std::unordered_map<
        size_t, std::unordered_map<std::string,
                                   std::vector<std::tuple<size_t, size_t>>>>
        m_AttributesIndicesInfo;

    /**
     * Put in BP buffer all attributes defined in an IO object.
     * Called by SerializeData function
     * @param io input containing attributes
     */
    void PutAttributes(core::IO &io);

    /**
     * Put in BP buffer attribute header, called from PutAttributeInData
     * specialized functions
     * @param attribute input
     * @param stats BP4 Stats
     * @param headerID  short string to identify block ("[AMD")
     * @return attribute length position
     */
    template <class T>
    size_t PutAttributeHeaderInData(const core::Attribute<T> &attribute,
                                    Stats<T> &stats, const char *headerID,
                                    const size_t headerIDLength) noexcept;

    /**
     * Called from WriteAttributeInData specialized functions
     * @param attribute input
     * @param stats BP4 stats
     * @param attributeLengthPosition
     */
    template <class T>
    void
    PutAttributeLengthInData(const core::Attribute<T> &attribute,
                             Stats<T> &stats,
                             const size_t attributeLengthPosition) noexcept;

    /**
     * Write a single attribute in data buffer, called from WriteAttributes
     * @param attribute input
     * @param stats BP4 Stats
     */
    template <class T>
    void PutAttributeInData(const core::Attribute<T> &attribute,
                            Stats<T> &stats) noexcept;

    /**
     * Writes attribute value in index characteristic value.
     * @param characteristicID
     * @param characteristicsCounter
     * @param attribute
     * @param buffer
     */
    template <class T>
    void
    PutAttributeCharacteristicValueInIndex(std::uint8_t &characteristicsCounter,
                                           const core::Attribute<T> &attribute,
                                           std::vector<char> &buffer) noexcept;

    /**
     * Write a single attribute in m_Metadata AttributesIndex, called from
     * WriteAttributes
     * @param attribute
     * @param stats BP4 stats
     */
    template <class T>
    void PutAttributeInIndex(const core::Attribute<T> &attribute,
                             const Stats<T> &stats) noexcept;

    /**
     * Get variable statistics
     * @param variable
     * @param isRowMajor
     * @return stats BP4 Stats
     */
    template <class T>
    Stats<T> GetBPStats(const bool singleValue,
                        const typename core::Variable<T>::Info &blockInfo,
                        const bool isRowMajor) noexcept;

    /** @return The position that holds the length of the variable entry
     * (metadat+data length). The actual lengths is know after
     * PutVariablePayload()
     */
    template <class T>
    size_t
    PutVariableMetadataInData(const core::Variable<T> &variable,
                              const typename core::Variable<T>::Info &blockInfo,
                              const Stats<T> &stats) noexcept;

    template <class T>
    void PutVariableMetadataInIndex(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const Stats<T> &stats, const bool isNew,
        SerialElementIndex &index) noexcept;

    template <class T>
    void PutVariableCharacteristics(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const Stats<T> &stats, std::vector<char> &buffer) noexcept;

    template <class T>
    void PutVariableCharacteristicsInData(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const Stats<T> &stats, std::vector<char> &buffer,
        size_t &position) noexcept;

    /**
     * Writes from &buffer[position]:  [2
     * bytes:string.length()][string.length():
     * string.c_str()]
     * @param name to be written in bp file
     * @param buffer metadata buffer
     */
    void PutNameRecord(const std::string name,
                       std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    void PutNameRecord(const std::string name, std::vector<char> &buffer,
                       size_t &position) noexcept;

    /**
     * Write a dimension record for a global variable used by
     * WriteVariableCommon
     * @param buffer
     * @param position
     * @param localDimensions
     * @param globalDimensions
     * @param offsets
     * @param addType true: for data buffers, false: for metadata buffer and
     * data
     * characteristic
     */
    void PutDimensionsRecord(const Dims &localDimensions,
                             const Dims &globalDimensions, const Dims &offsets,
                             std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    void PutDimensionsRecord(const Dims &localDimensions,
                             const Dims &globalDimensions, const Dims &offsets,
                             std::vector<char> &buffer, size_t &position,
                             const bool isCharacteristic = false) noexcept;

    /** Writes min max */
    template <class T>
    void PutBoundsRecord(const bool singleValue, const Stats<T> &stats,
                         uint8_t &characteristicsCounter,
                         std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    template <class T>
    void PutBoundsRecord(const bool singleValue, const Stats<T> &stats,
                         uint8_t &characteristicsCounter,
                         std::vector<char> &buffer, size_t &position) noexcept;

    /**
     * Write a characteristic value record to buffer
     * @param id
     * @param value
     * @param buffers
     * @param positions
     * @param characvteristicsCounter to be updated by 1
     * @param addLength true for data, false for metadata
     */
    template <class T>
    void PutCharacteristicRecord(const uint8_t characteristicID,
                                 uint8_t &characteristicsCounter,
                                 const T &value,
                                 std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    template <class T>
    void PutCharacteristicRecord(const uint8_t characteristicID,
                                 uint8_t &characteristicsCounter,
                                 const T &value, std::vector<char> &buffer,
                                 size_t &position) noexcept;

    /**
     * Returns corresponding serial index, if doesn't exists creates a
     * new one. Used for variables and attributes
     * @param name variable or attribute name to look for index
     * @param indices look up hash table of indices
     * @param isNew true: index is newly created, false: index already exists in
     * indices
     * @return reference to BP1Index in indices
     */
    SerialElementIndex &GetSerialElementIndex(
        const std::string &name,
        std::unordered_map<std::string, SerialElementIndex> &indices,
        bool &isNew) const noexcept;

    /**
     * Wraps up the data buffer serialization in m_HeapBuffer and fills the pg
     * length, vars count, vars
     * length and attributes count and attributes length
     * @param io object containing all attributes
     */
    void SerializeDataBuffer(core::IO &io) noexcept;

    /**
     * Puts minifooter into a bp buffer
     * @param pgIndexStart input offset
     * @param variablesIndexStart input offset
     * @param attributesIndexStart input offset
     * @param buffer  buffer to add the minifooter
     * @param position current buffer position
     * @param addSubfiles true: metadata file, false: data file
     */
    void PutMinifooter(const uint64_t pgIndexStart,
                       const uint64_t variablesIndexStart,
                       const uint64_t attributesIndexStart,
                       std::vector<char> &buffer, size_t &position,
                       const bool addSubfiles = false);

    /**
     * Used for PG index, aggregates without merging
     * @param index
     * @param count
     * @param comm
     * @param bufferSTL
     */
    void AggregateIndex(const SerialElementIndex &index, const size_t count,
                        MPI_Comm comm, BufferSTL &bufferSTL);

    /**
     * Collective operation to aggregate and merge (sort) indices (variables and
     * attributes)
     * @param indices has containing indices per unique variable/attribute name
     * @param comm communicator domain, allows reusing the function in
     * aggregation
     * @param bufferSTL buffer where merged indices will be placed (metadata or
     * data footer in aggregation)
     * @param isRankConstant true: use for attributes as values are constant
     * across all ranks, false: default used for variables as values  can vary
     * across ranks
     */
    void AggregateMergeIndex(
        const std::unordered_map<std::string, SerialElementIndex> &indices,
        MPI_Comm comm, BufferSTL &bufferSTL, const bool isRankConstant = false);

    void AggregateCollectiveMetadataIndices(MPI_Comm comm,
                                            BufferSTL &bufferSTL);

    /**
     * Returns a serialized buffer with all indices with format:
     * Rank (4 bytes), Buffer
     * @param indices input of all indices to be serialized
     * @return buffer with serialized indices
     */
    std::vector<char> SerializeIndices(
        const std::unordered_map<std::string, SerialElementIndex> &indices,
        MPI_Comm comm) const noexcept;

    /**
     * In rank=0, deserialize gathered indices
     * @param serializedIndices input gathered indices
     * @param comm establishes MPI domain
     * @param true: constant across ranks, no need to merge (attributes), false:
     * variable across ranks, can merge (variables)
     * @return hash[name][rank] = bp index buffer
     */
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
    DeserializeIndicesPerRankThreads(const std::vector<char> &serializedIndices,
                                     MPI_Comm comm,
                                     const bool isRankConstant) const noexcept;

    /** private function called by DeserializeIndicesPerRankThreads
     * in case of a single thread
     */
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
    DeserializeIndicesPerRankSingleThread(const std::vector<char> &serialized,
                                          MPI_Comm comm,
                                          const bool isRankConstant) const
        noexcept;
    /**
     * Merge indices by time step (default) and write to m_HeapBuffer.m_Metadata
     * @param nameRankIndices
     */
    void MergeSerializeIndices(
        const std::unordered_map<std::string, std::vector<SerialElementIndex>>
            &nameRankIndices,
        MPI_Comm comm, BufferSTL &bufferSTL);

    /**
     * Only merge indices of each time step and write to
     * m_HeapBuffer.m_Metadata, clear indices of current time step at the end of
     * each step
     * @param nameRankIndices
     */
    void MergeSerializeIndicesPerStep(
        const std::unordered_map<std::string, std::vector<SerialElementIndex>>
            &nameRankIndices,
        MPI_Comm comm, BufferSTL &bufferSTL);

    std::vector<char>
    SetCollectiveProfilingJSON(const std::string &rankLog) const;

    /**
     * Specialized for string and other types
     * @param variable input from which Payload is taken
     */
    template <class T>
    void PutPayloadInBuffer(const core::Variable<T> &variable,
                            const typename core::Variable<T>::Info &blockInfo,
                            const bool sourceRowMajor = true) noexcept;

    template <class T>
    void UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
                                           const DataTypes dataType,
                                           std::vector<char> &buffer);

    uint32_t GetFileIndex() const noexcept;

    size_t GetAttributesSizeInData(core::IO &io) const noexcept;

    template <class T>
    size_t GetAttributeSizeInData(const core::Attribute<T> &attribute) const
        noexcept;

    // Operations related functions
    template <class T>
    void PutCharacteristicOperation(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        std::vector<char> &buffer) noexcept;

    template <class T>
    void PutOperationPayloadInBuffer(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo);
};

#define declare_template_instantiation(T)                                      \
    extern template void BP4Serializer::PutVariablePayload(                    \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool) noexcept;                                                  \
                                                                               \
    extern template void BP4Serializer::PutVariableMetadata(                   \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP4_BP4Serializer_H_ */
