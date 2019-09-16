/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPSerializer.h
 *
 *  Created on: Sep 16, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPSERIALIZER_H_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPSERIALIZER_H_

#include "adios2/toolkit/format/bp/BPBase.h"

namespace adios2
{
namespace format
{

class BPSerializer : virtual public BPBase
{
public:
    BPSerializer(helper::Comm const &comm, const bool debugMode,
                 const uint8_t version);

    virtual ~BPSerializer() = default;

    /**
     * Serializes the data buffer and closes current process group
     * @param io : attributes written in first step
     * @param advanceStep true: advances step, false: doesn't advance
     */
    void SerializeData(core::IO &io, const bool advanceStep);

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
    std::vector<char> AggregateProfilingJSON(const std::string &rankLog) const;

protected:
    /** BP format version */
    const uint8_t m_Version;

    virtual void SerializeDataBuffer(core::IO &io) noexcept = 0;

    template <class T>
    void
    PutAttributeCharacteristicValueInIndex(uint8_t &characteristicsCounter,
                                           const core::Attribute<T> &attribute,
                                           std::vector<char> &buffer) noexcept;

    template <class T>
    void PutCharacteristicRecord(const uint8_t characteristicID,
                                 uint8_t &characteristicsCounter,
                                 const T &value,
                                 std::vector<char> &buffer) noexcept;

    template <class T>
    void PutCharacteristicRecord(const uint8_t characteristicID,
                                 uint8_t &characteristicsCounter,
                                 const T &value, std::vector<char> &buffer,
                                 size_t &position) noexcept;

    template <class T>
    void PutPayloadInBuffer(const core::Variable<T> &variable,
                            const typename core::Variable<T>::Info &blockInfo,
                            const bool sourceRowMajor) noexcept;

    void PutNameRecord(const std::string name,
                       std::vector<char> &buffer) noexcept;

    void PutNameRecord(const std::string name, std::vector<char> &buffer,
                       size_t &position) noexcept;

    void PutDimensionsRecord(const Dims &localDimensions,
                             const Dims &globalDimensions, const Dims &offsets,
                             std::vector<char> &buffer) noexcept;

    void PutDimensionsRecord(const Dims &localDimensions,
                             const Dims &globalDimensions, const Dims &offsets,
                             std::vector<char> &buffer, size_t &position,
                             const bool isCharacteristic = false) noexcept;

    void PutMinifooter(const uint64_t pgIndexStart,
                       const uint64_t variablesIndexStart,
                       const uint64_t attributesIndexStart,
                       std::vector<char> &buffer, size_t &position,
                       const bool addSubfiles = false);

    void UpdateOffsetsInMetadata();

private:
    template <class T>
    void UpdateIndexOffsetsCharacteristics(size_t &currentPosition,
                                           const DataTypes dataType,
                                           std::vector<char> &buffer);
};

} // end namespace format
} // end namespace adios2

#include "BPSerializer.inl"

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_BPSERIALIZER_H_ */
