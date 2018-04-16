/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Serializer.h
 *
 *  Created on: Jan 24, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_BP3SERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP3_BP3SERIALIZER_H_

#include <mutex>

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"
#include "adios2/core/Variable.h"
#include "adios2/toolkit/format/bp3/BP3Base.h"

namespace adios2
{
namespace format
{

class BP3Serializer : public BP3Base
{

public:
    std::vector<std::string> m_DeferredVariables;
    size_t m_DeferredVariablesDataSize = 0;
    /**
     * Unique constructor
     * @param mpiComm MPI communicator for BP1 Aggregator
     * @param debug true: extra checks
     */
    BP3Serializer(MPI_Comm mpiComm, const bool debugMode = false);

    ~BP3Serializer() = default;

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
    void PutVariableMetadata(const Variable<T> &variable) noexcept;

    /**
     * Put in buffer variable payload. Expensive part.
     * @param variable payload input from m_PutValues
     */
    template <class T>
    void PutVariablePayload(const Variable<T> &variable) noexcept;

    /**
     *  Serializes data buffer and close current process group
     * @param io : attributes written in first step
     * @param advanceStep true: advances step, false: doesn't advance
     */
    void SerializeData(IO &io, const bool advanceStep = false);

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
     */
    void CloseData(IO &io);

    /**
     * Closes bp buffer for streaming mode...must reset metadata for the next
     * step
     * @param io object containing all attributes
     * @param addMetadata true: process metadata and add to data buffer
     * (minifooter)
     */
    void CloseStream(IO &io, const bool addMetadata = true);
    void CloseStream(IO &io, size_t &metadataStart, size_t &metadataCount,
                     const bool addMetadata = true);

    void ResetIndices();

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
     * Creates the final collective Metadata buffer in m_HeapBuffer.m_Metadata
     * from all ranks
     */
    void AggregateCollectiveMetadata();

    /**
     * Updates data absolute position based on data from other producers in
     * aggregation
     */
    void AggregatorsUpdateDataAbsolutePosition();

    /** Updates variable and payload offsets in metadata characteristics with
     * the updated Buffer m_DataAbsolutePosition. This is a local (non-MPI)
     * operation */
    void AggregatorsUpdateOffsetsInMetadata();

    /** Sends aggregation data in non-blocking mode according to the strategy
     * used */
    void AggregatorsISend(const int step);

    /**
     * reference to buffer ready to be consumed (used by transports via a
     * transport manager).
     * This function should be only used by aggregator with rank 0.
     * @return reference to buffer ready for consumption
     */
    BufferSTL &AggregatorConsumerBuffer();

    /** Receive aggregation data in non-blocking mode according to the strategy
     * used */
    void AggregatorsIReceive(const int step);

private:
    /** BP format version */
    const uint8_t m_Version = 3;

    static std::mutex m_Mutex;

    /**
     * Put in BP buffer all attributes defined in an IO object.
     * Called by SerializeData function
     * @param io input containing attributes
     */
    void PutAttributes(IO &io);

    /**
     * Put in BP buffer attribute header, called from PutAttributeInData
     * specialized functions
     * @param attribute input
     * @param stats
     * @return attribute length position
     */
    template <class T>
    size_t PutAttributeHeaderInData(const Attribute<T> &attribute,
                                    Stats<T> &stats) noexcept;

    /**
     * Called from WriteAttributeInData specialized functions
     * @param attribute input
     * @param stats
     * @param attributeLengthPosition
     */
    template <class T>
    void
    PutAttributeLengthInData(const Attribute<T> &attribute, Stats<T> &stats,
                             const size_t attributeLengthPosition) noexcept;

    /**
     * Write a single attribute in data buffer, called from WriteAttributes
     * @param attribute input
     * @param stats
     */
    template <class T>
    void PutAttributeInData(const Attribute<T> &attribute,
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
                                           const Attribute<T> &attribute,
                                           std::vector<char> &buffer) noexcept;

    /**
     * Write a single attribute in m_Metadata AttributesIndex, called from
     * WriteAttributes
     * @param attribute
     * @param stats
     */
    template <class T>
    void PutAttributeInIndex(const Attribute<T> &attribute,
                             const Stats<T> &stats) noexcept;

    /**
     * Get variable statistics
     * @param variable
     * @return stats
     */
    template <class T>
    Stats<typename TypeInfo<T>::ValueType>
    GetStats(const Variable<T> &variable) noexcept;

    template <class T>
    void PutVariableMetadataInData(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats) noexcept;

    template <class T>
    void PutVariableMetadataInIndex(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats, const bool isNew,
        SerialElementIndex &index) noexcept;

    template <class T>
    void PutVariableCharacteristics(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats,
        std::vector<char> &buffer) noexcept;

    template <class T>
    void PutVariableCharacteristics(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats,
        std::vector<char> &buffer, size_t &position) noexcept;

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
    void PutBoundsRecord(const bool isScalar, const Stats<T> &stats,
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
    void SerializeDataBuffer(IO &io) noexcept;

    void PutMinifooter(const uint64_t pgIndexStart,
                       const uint64_t variablesIndexStart,
                       const uint64_t attributesIndexStart,
                       std::vector<char> &buffer, size_t &position,
                       const bool addSubfiles = false);

    /**
     * Used for PG index, aggregates without merging
     * @param index input
     * @param count total number of indices
     */
    void AggregateIndex(const SerialElementIndex &index, const size_t count);

    /**
     * Collective operation to aggregate and merge (sort) indices (variables and
     * attributes)
     * @param indices
     */
    void AggregateMergeIndex(
        const std::unordered_map<std::string, SerialElementIndex> &indices);

    /**
     * Returns a serialized buffer with all indices with format:
     * Rank (4 bytes), Buffer
     * @param indices input of all indices to be serialized
     * @return buffer with serialized indices
     */
    std::vector<char> SerializeIndices(
        const std::unordered_map<std::string, SerialElementIndex> &indices)
        const noexcept;

    /**
     * In rank=0, deserialize gathered indices
     * @param serializedIndices input gathered indices
     * @return hash[name][rank] = bp index buffer
     */
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
    DeserializeIndicesPerRankThreads(
        const std::vector<char> &serializedIndices) const noexcept;

    /**
     * Merge indices by time step (default) and write to m_HeapBuffer.m_Metadata
     * @param nameRankIndices
     */
    void MergeSerializeIndices(
        const std::unordered_map<std::string, std::vector<SerialElementIndex>>
            &nameRankIndices);

    std::vector<char>
    SetCollectiveProfilingJSON(const std::string &rankLog) const;

    /**
     * Specialized for string and other types
     * @param variable input from which Payload is taken
     */
    template <class T>
    void PutPayloadInBuffer(const Variable<T> &variable) noexcept;

    /**
     * Updates variable and payload offsets with buffer m_DataAbsolutePosition
     * @param index
     */
    void UpdateIndexOffsets(SerialElementIndex &index);

    template <class T>
    void UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
                                           const DataTypes dataType,
                                           std::vector<char> &buffer);

    uint32_t GetFileIndex() const noexcept;
};

#define declare_template_instantiation(T)                                      \
    extern template void BP3Serializer::PutVariablePayload(                    \
        const Variable<T> &variable) noexcept;                                 \
                                                                               \
    extern template void BP3Serializer::PutVariableMetadata(                   \
        const Variable<T> &variable) noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP3_BP3Serializer_H_ */
