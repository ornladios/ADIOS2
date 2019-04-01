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

#include "adios2/core/Attribute.h"
#include "adios2/core/IO.h"
#include "adios2/toolkit/format/bp3/BP3Base.h"

namespace adios2
{
namespace format
{

class BP3Serializer : public BP3Base
{

public:
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
     */
    template <class T>
    void PutVariableMetadata(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const bool sourceRowMajor = true,
        typename core::Variable<T>::Span *span = nullptr) noexcept;

    /**
     * Put in buffer variable payload. Expensive part.
     * @param variable payload input from m_PutValues
     */
    template <class T>
    void PutVariablePayload(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const bool sourceRowMajor = true,
        typename core::Variable<T>::Span *span = nullptr) noexcept;

    template <class T>
    void PutSpanMetadata(const core::Variable<T> &variable,
                         const typename core::Variable<T>::Span &span) noexcept;

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
     */
    void CloseData(core::IO &io);

    /**
     * Closes bp buffer for streaming mode...must reset metadata for the next
     * step
     * @param io object containing all attributes
     * @param addMetadata true: process metadata and add to data buffer
     * (minifooter)
     */
    void CloseStream(core::IO &io, const bool addMetadata = true);
    void CloseStream(core::IO &io, size_t &metadataStart, size_t &metadataCount,
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
    const uint8_t m_Version = 3;

    static std::mutex m_Mutex;

    /** aggregate pg rank indices */
    std::vector<char> m_PGRankIndices;
    /** deserialized variable indices per rank (vector index) */
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
        m_VariableRankIndices;
    /** deserialized attribute indices per rank (vector index) */
    std::unordered_map<std::string, std::vector<SerialElementIndex>>
        m_AttributesRankIndices;

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
     * @param stats BP3 Stats
     * @return attribute length position
     */
    template <class T>
    size_t PutAttributeHeaderInData(const core::Attribute<T> &attribute,
                                    Stats<T> &stats) noexcept;

    /**
     * Called from WriteAttributeInData specialized functions
     * @param attribute input
     * @param stats BP3 stats
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
     * @param stats BP3 Stats
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
     * @param stats BP3 stats
     */
    template <class T>
    void PutAttributeInIndex(const core::Attribute<T> &attribute,
                             const Stats<T> &stats) noexcept;

    /**
     * Get variable statistics
     * @param variable
     * @param isRowMajor
     * @return stats BP3 Stats
     */
    template <class T>
    Stats<T> GetBPStats(const bool singleValue,
                        const typename core::Variable<T>::Info &blockInfo,
                        const bool isRowMajor) noexcept;

    template <class T>
    void
    PutVariableMetadataInData(const core::Variable<T> &variable,
                              const typename core::Variable<T>::Info &blockInfo,
                              const Stats<T> &stats) noexcept;

    template <class T>
    void PutVariableMetadataInIndex(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const Stats<T> &stats, const bool isNew, SerialElementIndex &index,
        typename core::Variable<T>::Span *span) noexcept;

    template <class T>
    void PutVariableCharacteristics(
        const core::Variable<T> &variable,
        const typename core::Variable<T>::Info &blockInfo,
        const Stats<T> &stats, std::vector<char> &buffer,
        typename core::Variable<T>::Span *span) noexcept;

    template <class T>
    void PutVariableCharacteristics(
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
     * Refactored function that reduces the communication at scale by just
     * calling a Gather/GatherV pair once for all indices
     * @param comm
     * @param bufferSTL
     * @param inMetadataBuffer
     * @return contains indices positions in buffer
     */
    std::vector<size_t>
    AggregateCollectiveMetadataIndices(MPI_Comm comm, BufferSTL &bufferSTL);

    /**
     * Merge indices by time step (default) and write to m_HeapBuffer.m_Metadata
     * @param nameRankIndices
     */
    void MergeSerializeIndices(
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

    void SetDataOffset(uint64_t &offset) noexcept;

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
    extern template void BP3Serializer::PutVariableMetadata(                   \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool, typename core::Variable<T>::Span *) noexcept;              \
                                                                               \
    extern template void BP3Serializer::PutVariablePayload(                    \
        const core::Variable<T> &, const typename core::Variable<T>::Info &,   \
        const bool, typename core::Variable<T>::Span *) noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
    extern template void BP3Serializer::PutSpanMetadata(                       \
        const core::Variable<T> &,                                             \
        const typename core::Variable<T>::Span &) noexcept;

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_UTILITIES_FORMAT_BP3_BP3Serializer_H_ */
