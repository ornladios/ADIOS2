/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1.h
 *
 *  Created on: Jan 24, 2017
 *      Author: wfg
 */

#ifndef ADIOS2_UTILITIES_FORMAT_BP1_BP1WRITER_H_
#define ADIOS2_UTILITIES_FORMAT_BP1_BP1WRITER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::count, std::copy, std::for_each
#include <cmath>     //std::ceil
#include <cstring>   //std::memcpy
/// \endcond

#include "adios2/ADIOSConfig.h"
#include "adios2/ADIOSMacros.h"
#include "adios2/ADIOSTypes.h"
#include "adios2/capsule/heap/STLVector.h"
#include "adios2/core/Variable.h"
#include "adios2/core/adiosFunctions.h"
#include "adios2/core/adiosTemplates.h"
#include "adios2/utilities/format/bp1/BP1Base.h"
#include "adios2/utilities/format/bp1/BP1Structs.h"

namespace adios
{
namespace format
{

class BP1Writer : public BP1Base
{

public:
    /**
     * Unique constructor
     * @param mpiComm MPI communicator for BP1 Aggregator
     * @param debug true: extra checks
     */
    BP1Writer(MPI_Comm mpiComm, const bool debugMode = false);

    virtual ~BP1Writer() = default;

    /**
     * Writes a process group index PGIndex and list of methods (from
     * transports). Done at Open or Advance.
     * @param isFortran
     * @param name group name, taking the rank
     * @param processID
     * @param transports
     */
    void WriteProcessGroupIndex(
        const bool isFortran, const std::string name, const uint32_t processID,
        const std::vector<std::shared_ptr<Transport>> &transports) noexcept;

    /**
     *
     * @param variable
     * @return
     * -1: allocation failed,
     *  0: no allocation needed,
     *  1: reallocation is sucessful
     *  2: need a transport flush
     */
    template <class T>
    ResizeResult ResizeBuffer(const Variable<T> &variable);

    /**
     * Write metadata for a given variable
     * @param variable
     */
    template <class T>
    void WriteVariableMetadata(const Variable<T> &variable) noexcept;

    /**
     * Expensive part this is only for heap buffers need to adapt to vector of
     * capsules
     * @param variable
     */
    template <class T>
    void WriteVariablePayload(const Variable<T> &variable) noexcept;

    /** Flattens data buffer */
    void Advance();

    /**
     * Function that sets metadata (if first close) and writes to a single
     * transport
     * @param metadataSet current rank metadata set
     * @param heap contains data buffer
     * @param transport does a write after data and metadata is setup
     * @param isFirstClose true: metadata has been set and aggregated
     * @param doAggregation true: for N-to-M, false: for N-to-N
     */
    void Close(Transport &transport, bool &isFirstClose,
               const bool doAggregation) noexcept;

    void WriteProfilingLogFile(
        const std::string &name, const unsigned int rank,
        const std::vector<std::shared_ptr<Transport>> &transports) noexcept;

private:
    /** BP format version */
    const std::uint8_t m_Version = 3;

    /**
     * Calculates the Process Index size in bytes according to the BP format,
     * including list of method with no parameters (for now)
     * @param name
     * @param timeStepName
     * @param numberOfTransports
     * @return size of pg index
     */
    size_t GetProcessGroupIndexSize(const std::string name,
                                    const std::string timeStepName,
                                    const size_t numberOfTransports) const
        noexcept;

    /**
     * Returns the estimated variable index size
     * @param variable
     */
    template <class T>
    size_t GetVariableIndexSize(const Variable<T> &variable) const noexcept;

    /**
     * Get variable statistics
     * @param variable
     * @return stats
     */
    template <class T>
    Stats<typename TypeInfo<T>::ValueType>
    GetStats(const Variable<T> &variable) const noexcept;

    template <class T>
    void WriteVariableMetadataInData(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats) noexcept;

    template <class T>
    void WriteVariableMetadataInIndex(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats, const bool isNew,
        BP1Index &index) noexcept;

    template <class T>
    void WriteVariableCharacteristics(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats,
        std::vector<char> &buffer) noexcept;

    template <class T>
    void WriteVariableCharacteristics(
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
    void WriteNameRecord(const std::string name,
                         std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    void WriteNameRecord(const std::string name, std::vector<char> &buffer,
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
    void WriteDimensionsRecord(const std::vector<size_t> &localDimensions,
                               const std::vector<size_t> &globalDimensions,
                               const std::vector<size_t> &offsets,
                               std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    void WriteDimensionsRecord(const std::vector<size_t> &localDimensions,
                               const std::vector<size_t> &globalDimensions,
                               const std::vector<size_t> &offsets,
                               const unsigned int skip,
                               std::vector<char> &buffer,
                               size_t &position) noexcept;

    /** Writes min max */
    template <class T>
    void WriteBoundsRecord(const bool isScalar, const Stats<T> &stats,
                           uint8_t &characteristicsCounter,
                           std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    template <class T>
    void WriteBoundsRecord(const bool isScalar, const Stats<T> &stats,
                           uint8_t &characteristicsCounter,
                           std::vector<char> &buffer,
                           size_t &position) noexcept;

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
    void WriteCharacteristicRecord(const uint8_t characteristicID,
                                   uint8_t &characteristicsCounter,
                                   const T &value,
                                   std::vector<char> &buffer) noexcept;

    /** Overloaded version for data buffer */
    template <class T>
    void WriteCharacteristicRecord(const uint8_t characteristicID,
                                   uint8_t &characteristicsCounter,
                                   const T &value, std::vector<char> &buffer,
                                   size_t &position) noexcept;

    /**
     * Returns corresponding index of type BP1Index, if doesn't exists creates a
     * new one. Used for variables and attributes
     * @param name variable or attribute name to look for index
     * @param indices look up hash table of indices
     * @param isNew true: index is newly created, false: index already exists in
     * indices
     * @return reference to BP1Index in indices
     */
    BP1Index &GetBP1Index(const std::string name,
                          std::unordered_map<std::string, BP1Index> &indices,
                          bool &isNew) const noexcept;

    /**
     * Flattens the data and fills the pg length, vars count, vars length and
     * attributes
     * @param metadataSet
     * @param buffer
     */
    void FlattenData() noexcept;

    /**
     * Flattens the metadata indices into a single metadata buffer in capsule
     * @param metadataSet
     * @param buffer
     */
    void FlattenMetadata() noexcept;

    /**
     * Writes the ADIOS log information (buffering, open, write and close) for a
     * rank process
     * @param rank current rank
     * @param metadataSet contains Profile info for buffering
     * @param transports  contains Profile info for transport open, writes and
     * close
     * @return string for this rank that will be aggregated into profiling.log
     */
    std::string GetRankProfilingLog(
        const unsigned int rank,
        const std::vector<std::shared_ptr<Transport>> &transports) noexcept;
};

#define declare_template_instantiation(T)                                      \
    extern template BP1Writer::ResizeResult BP1Writer::ResizeBuffer(           \
        const Variable<T> &variable);                                          \
                                                                               \
    extern template void BP1Writer::WriteVariablePayload(                      \
        const Variable<T> &variable) noexcept;                                 \
                                                                               \
    extern template void BP1Writer::WriteVariableMetadata(                     \
        const Variable<T> &variable) noexcept;

ADIOS_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios

#endif /* ADIOS2_UTILITIES_FORMAT_BP1_BP1WRITER_H_ */
