/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1.h
 *
 *  Created on: Jan 24, 2017
 *      Author: wfg
 */

#ifndef BP1WRITER_H_
#define BP1WRITER_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::count, std::copy, std::for_each
#include <cmath>     //std::ceil
#include <cstring>   //std::memcpy
/// \endcond

#include "ADIOSMacros.h"
#include "ADIOSTypes.h"
#include "utilities/format/bp1/BP1Base.h"
#include "utilities/format/bp1/BP1Structs.h"

#include "capsule/heap/STLVector.h"
#include "core/Variable.h"
#include "functions/adiosFunctions.h"
#include "functions/adiosTemplates.h"

namespace adios
{
namespace format
{

class BP1Writer : public BP1Base
{

public:
    unsigned int m_Threads = 1; ///< thread operations in large array (min,max)
    unsigned int m_Verbosity = 0; ///< statistics verbosity, only 0 is supported
    float m_GrowthFactor = 1.5;   ///< memory growth factor
    const std::uint8_t m_Version = 3; ///< BP format version

    /**
     * Calculates the Process Index size in bytes according to the BP format,
     * including list of method with no parameters (for now)
     * @param name
     * @param timeStepName
     * @param numberOfTransports
     * @return size of pg index
     */
    std::size_t GetProcessGroupIndexSize(
        const std::string name, const std::string timeStepName,
        const std::size_t numberOfTransports) const noexcept;

    /**
     * Writes a process group index PGIndex and list of methods (from
     * transports),
     * done at Open or aggregation of new time step
     * Version that operates on a single heap buffer and metadataset.
     * @param isFortran
     * @param name
     * @param processID
     * @param transports
     * @param buffer
     * @param metadataSet
     */
    void WriteProcessGroupIndex(
        const bool isFortran, const std::string name,
        const std::uint32_t processID,
        const std::vector<std::shared_ptr<Transport>> &transports,
        capsule::STLVector &heap, BP1MetadataSet &metadataSet) const noexcept;

    /**
     * Returns the estimated variable index size
     * @param group
     * @param variableName
     * @param variable
     * @param verbosity
     * @return variable index size
     */
    template <class T>
    std::size_t GetVariableIndexSize(const Variable<T> &variable) const
        noexcept;

    /**
     * Write metadata for a given variable
     * @param variable
     * @param heap
     * @param metadataSet
     */
    template <class T>
    void WriteVariableMetadata(const Variable<T> &variable,
                               capsule::STLVector &heap,
                               BP1MetadataSet &metadataSet) const noexcept;

    /**
     * Expensive part this is only for heap buffers need to adapt to vector of
     * capsules
     * @param variable
     * @param buffer
     */
    template <class T>
    void WriteVariablePayload(const Variable<T> &variable,
                              capsule::STLVector &heap,
                              const unsigned int nthreads = 1) const noexcept;

    /**
     * Flattens data
     * @param metadataSet
     * @param buffer
     */
    void Advance(BP1MetadataSet &metadataSet, capsule::STLVector &buffer);

    /**
     * Function that sets metadata (if first close) and writes to a single
     * transport
     * @param metadataSet current rank metadata set
     * @param heap contains data buffer
     * @param transport does a write after data and metadata is setup
     * @param isFirstClose true: metadata has been set and aggregated
     * @param doAggregation true: for N-to-M, false: for N-to-N
     */
    void Close(BP1MetadataSet &metadataSet, capsule::STLVector &heap,
               Transport &transport, bool &isFirstClose,
               const bool doAggregation) const noexcept;

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
        const int rank, const BP1MetadataSet &metadataSet,
        const std::vector<std::shared_ptr<Transport>> &transports) const
        noexcept;

private:
    template <class T>
    void WriteVariableMetadataInData(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats,
        capsule::STLVector &heap) const noexcept;

    template <class T>
    void WriteVariableMetadataInIndex(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats, const bool isNew,
        BP1Index &index) const noexcept;

    template <class T>
    void WriteVariableCharacteristics(
        const Variable<T> &variable,
        const Stats<typename TypeInfo<T>::ValueType> &stats,
        std::vector<char> &buffer, const bool addLength = false) const noexcept;

    /**
     * Writes from &buffer[position]:  [2
     * bytes:string.length()][string.length():
     * string.c_str()]
     * @param name
     * @param buffer
     * @param position
     */
    void WriteNameRecord(const std::string name,
                         std::vector<char> &buffer) const noexcept;

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
    void WriteDimensionsRecord(std::vector<char> &buffer,
                               const std::vector<std::size_t> &localDimensions,
                               const std::vector<std::size_t> &globalDimensions,
                               const std::vector<std::size_t> &offsets,
                               const unsigned int skip,
                               const bool addType = false) const noexcept;

    /**
     * Get variable statistics
     * @param variable
     * @return stats
     */
    template <class T>
    Stats<typename TypeInfo<T>::ValueType>
    GetStats(const Variable<T> &variable) const noexcept;

    template <class T>
    void WriteBoundsRecord(const bool isScalar, const Stats<T> &stats,
                           std::vector<char> &buffer,
                           std::uint8_t &characteristicsCounter,
                           const bool addLength) const noexcept;

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
    void WriteCharacteristicRecord(const std::uint8_t characteristicID,
                                   const T &value, std::vector<char> &buffer,
                                   std::uint8_t &characteristicsCounter,
                                   const bool addLength = false) const noexcept;

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
    void FlattenData(BP1MetadataSet &metadataSet,
                     capsule::STLVector &buffer) const noexcept;

    /**
     * Flattens the metadata indices into a single metadata buffer in capsule
     * @param metadataSet
     * @param buffer
     */
    void FlattenMetadata(BP1MetadataSet &metadataSet,
                         capsule::STLVector &buffer) const noexcept;
};

#define declare_template_instantiation(T)                                      \
    extern template void BP1Writer::WriteVariablePayload(                      \
        const Variable<T> &variable, capsule::STLVector &heap,                 \
        const unsigned int nthreads) const noexcept;                           \
                                                                               \
    extern template void BP1Writer::WriteVariableMetadata(                     \
        const Variable<T> &variable, capsule::STLVector &heap,                 \
        BP1MetadataSet &metadataSet) const noexcept;

ADIOS_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios

#endif /* BP1WRITER_H_ */
