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

#include "BP1.h"
#include "capsule/heap/STLVector.h"
#include "core/Capsule.h"
#include "core/Profiler.h"
#include "core/Variable.h"
#include "functions/adiosFunctions.h"
#include "functions/adiosTemplates.h"

namespace adios
{
namespace format
{

class BP1Writer : public BP1
{

public:
  unsigned int m_Threads =
      1; ///< number of threads for thread operations in large array (min,max)
  unsigned int m_Verbosity =
      0; ///< statistics verbosity, can change if redefined in Engine method.
  float m_GrowthFactor =
      1.5; ///< memory growth factor, can change if redefined in Engine method.
  const std::uint8_t m_Version = 3; ///< BP format version

  /**
   * Calculates the Process Index size in bytes according to the BP format,
   * including list of method with no parameters (for now)
   * @param name
   * @param timeStepName
   * @param numberOfTransports
   * @return size of pg index
   */
  std::size_t
  GetProcessGroupIndexSize(const std::string name,
                           const std::string timeStepName,
                           const std::size_t numberOfTransports) const noexcept;

  /**
   * Writes a process group index PGIndex and list of methods (from transports),
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
  size_t GetVariableIndexSize(const Variable<T> &variable) const noexcept
  {
    // size_t indexSize = varEntryLength + memberID + lengthGroupName +
    // groupName + lengthVariableName + lengthOfPath + path + datatype
    size_t indexSize = 23; // without characteristics
    indexSize += variable.m_Name.size();

    // characteristics 3 and 4, check variable number of dimensions
    const std::size_t dimensions =
        variable.DimensionsSize(); // number of commas in CSV + 1
    indexSize += 28 * dimensions;  // 28 bytes per dimension
    indexSize += 1;                // id

    // characteristics, offset + payload offset in data
    indexSize += 2 * (1 + 8);
    // characteristic 0, if scalar add value, for now only allowing string
    if (dimensions == 1)
    {
      indexSize += sizeof(T);
      indexSize += 1; // id
      // must have an if here
      indexSize += 2 + variable.m_Name.size();
      indexSize += 1; // id
    }

    // characteristic statistics
    if (m_Verbosity == 0) // default, only min and max
    {
      indexSize += 2 * (sizeof(T) + 1);
      indexSize += 1 + 1; // id
    }

    return indexSize + 12; /// extra 12 bytes in case of attributes
    // need to add transform characteristics
  }

  /**
   * Version for primitive types (except std::complex<T>)
   * @param variable
   * @param heap
   * @param metadataSet
   */
  template <class T>
  inline void WriteVariableMetadata(const Variable<T> &variable,
                                    capsule::STLVector &heap,
                                    BP1MetadataSet &metadataSet) const noexcept
  {
    Stats<T> stats = GetStats(variable);
    WriteVariableMetadataCommon(variable, stats, heap, metadataSet);
  }

  /**
   * Overloaded version for std::complex<T> variables
   * @param variable
   * @param heap
   * @param metadataSet
   */
  template <class T>
  void WriteVariableMetadata(const Variable<std::complex<T>> &variable,
                             capsule::STLVector &heap,
                             BP1MetadataSet &metadataSet) const noexcept
  {
    Stats<T> stats = GetStats(variable);
    WriteVariableMetadataCommon(variable, stats, heap, metadataSet);
  }

  /**
   * Expensive part this is only for heap buffers need to adapt to vector of
   * capsules
   * @param variable
   * @param buffer
   */
  template <class T>
  void WriteVariablePayload(const Variable<T> &variable,
                            capsule::STLVector &heap,
                            const unsigned int nthreads = 1) const noexcept
  {
    // EXPENSIVE part, might want to use threads if large, serial for now
    CopyToBuffer(heap.m_Data, variable.m_AppValues, variable.TotalSize());
    heap.m_DataAbsolutePosition += variable.PayLoadSize();
  }

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
      const std::vector<std::shared_ptr<Transport>> &transports) const noexcept;

private:
  template <class T, class U>
  void WriteVariableMetadataCommon(const Variable<T> &variable, Stats<U> &stats,
                                   capsule::STLVector &heap,
                                   BP1MetadataSet &metadataSet) const noexcept
  {
    stats.TimeIndex = metadataSet.TimeStep;

    // Get new Index or point to existing index
    bool isNew = true; // flag to check if variable is new
    BP1Index &varIndex =
        GetBP1Index(variable.m_Name, metadataSet.VarsIndices, isNew);
    stats.MemberID = varIndex.MemberID;

    // write metadata header in data and extract offsets
    stats.Offset = heap.m_DataAbsolutePosition;
    WriteVariableMetadataInData(variable, stats, heap);
    stats.PayloadOffset = heap.m_DataAbsolutePosition;

    // write to metadata  index
    WriteVariableMetadataInIndex(variable, stats, isNew, varIndex);

    ++metadataSet.DataPGVarsCount;
  }

  template <class T, class U>
  void WriteVariableMetadataInData(const Variable<T> &variable,
                                   const Stats<U> &stats,
                                   capsule::STLVector &heap) const noexcept
  {
    auto &buffer = heap.m_Data;

    const std::size_t varLengthPosition =
        buffer.size(); // capture initial position for variable length
    buffer.insert(buffer.end(), 8, 0);              // skip var length (8)
    CopyToBuffer(buffer, &stats.MemberID);          // memberID
    WriteNameRecord(variable.m_Name, buffer);       // variable name
    buffer.insert(buffer.end(), 2, 0);              // skip path
    const std::uint8_t dataType = GetDataType<T>(); // dataType
    CopyToBuffer(buffer, &dataType);
    constexpr char no = 'n'; // isDimension
    CopyToBuffer(buffer, &no);

    // write variable dimensions
    const std::uint8_t dimensions = variable.m_Dimensions.size();
    CopyToBuffer(buffer, &dimensions); // count
    std::uint16_t dimensionsLength =
        27 * dimensions; // 27 is from 9 bytes for each: var y/n + local, var
                         // y/n + global dimension, var y/n + global offset,
                         // changed for characteristic
    CopyToBuffer(buffer, &dimensionsLength); // length
    WriteDimensionsRecord(buffer, variable.m_Dimensions,
                          variable.m_GlobalDimensions, variable.m_GlobalOffsets,
                          18, true);

    // CHARACTERISTICS
    WriteVariableCharacteristics(variable, stats, buffer, true);

    // Back to varLength including payload size
    const std::uint64_t varLength = buffer.size() - varLengthPosition +
                                    variable.PayLoadSize() -
                                    8;                   // remove its own size
    CopyToBuffer(buffer, varLengthPosition, &varLength); // length

    heap.m_DataAbsolutePosition +=
        buffer.size() - varLengthPosition; // update absolute position to be
                                           // used as payload position
  }

  template <class T, class U>
  void WriteVariableMetadataInIndex(const Variable<T> &variable,
                                    const Stats<U> &stats, const bool isNew,
                                    BP1Index &index) const noexcept
  {
    auto &buffer = index.Buffer;

    if (isNew ==
        true) // write variable header (might be shared with attributes index)
    {
      buffer.insert(buffer.end(), 4, 0); // skip var length (4)
      CopyToBuffer(buffer, &stats.MemberID);
      buffer.insert(buffer.end(), 2, 0); // skip group name
      WriteNameRecord(variable.m_Name, buffer);
      buffer.insert(buffer.end(), 2, 0); // skip path

      const std::uint8_t dataType = GetDataType<T>();
      CopyToBuffer(buffer, &dataType);

      // Characteristics Sets Count in Metadata
      index.Count = 1;
      CopyToBuffer(buffer, &index.Count);
    }
    else // update characteristics sets count
    {
      const std::size_t characteristicsSetsCountPosition =
          15 + variable.m_Name.size();
      ++index.Count;
      CopyToBuffer(buffer, characteristicsSetsCountPosition,
                   &index.Count); // test
    }

    WriteVariableCharacteristics(variable, stats, buffer);
  }

  template <class T, class U>
  void WriteVariableCharacteristics(const Variable<T> &variable,
                                    const Stats<U> &stats,
                                    std::vector<char> &buffer,
                                    const bool addLength = false) const noexcept
  {
    const std::size_t characteristicsCountPosition =
        buffer.size(); // very important to track as writer is going back to
                       // this position
    buffer.insert(buffer.end(), 5,
                  0); // skip characteristics count(1) + length (4)
    std::uint8_t characteristicsCounter = 0;

    // DIMENSIONS
    std::uint8_t characteristicID = characteristic_dimensions;
    CopyToBuffer(buffer, &characteristicID);
    const std::uint8_t dimensions = variable.m_Dimensions.size();

    if (addLength == true)
    {
      const std::int16_t lengthOfDimensionsCharacteristic =
          24 * dimensions +
          3; // 24 = 3 local, global, global offset x 8 bytes/each
      CopyToBuffer(buffer, &lengthOfDimensionsCharacteristic);
    }

    CopyToBuffer(buffer, &dimensions); // count
    const std::uint16_t dimensionsLength = 24 * dimensions;
    CopyToBuffer(buffer, &dimensionsLength); // length
    WriteDimensionsRecord(buffer, variable.m_Dimensions,
                          variable.m_GlobalDimensions, variable.m_GlobalOffsets,
                          16, addLength);
    ++characteristicsCounter;

    // VALUE for SCALAR or STAT min, max for ARRAY
    WriteBoundsRecord(variable.m_IsScalar, stats, buffer,
                      characteristicsCounter, addLength);
    // TIME INDEX
    WriteCharacteristicRecord(characteristic_time_index, stats.TimeIndex,
                              buffer, characteristicsCounter, addLength);

    if (addLength == false) // only in metadata offset and payload offset
    {
      WriteCharacteristicRecord(characteristic_offset, stats.Offset, buffer,
                                characteristicsCounter);
      WriteCharacteristicRecord(characteristic_payload_offset,
                                stats.PayloadOffset, buffer,
                                characteristicsCounter);
    }
    // END OF CHARACTERISTICS

    // Back to characteristics count and length
    CopyToBuffer(buffer, characteristicsCountPosition,
                 &characteristicsCounter); // count (1)
    const std::uint32_t characteristicsLength =
        buffer.size() - characteristicsCountPosition - 4 -
        1; // remove its own length (4 bytes) + characteristic counter ( 1 byte
           // )
    CopyToBuffer(buffer, characteristicsCountPosition + 1,
                 &characteristicsLength); // length
  }

  /**
   * Writes from &buffer[position]:  [2 bytes:string.length()][string.length():
   * string.c_str()]
   * @param name
   * @param buffer
   * @param position
   */
  void WriteNameRecord(const std::string name, std::vector<char> &buffer) const
      noexcept;

  /**
   * Write a dimension record for a global variable used by WriteVariableCommon
   * @param buffer
   * @param position
   * @param localDimensions
   * @param globalDimensions
   * @param globalOffsets
   * @param addType true: for data buffers, false: for metadata buffer and data
   * characteristic
   */
  void WriteDimensionsRecord(std::vector<char> &buffer,
                             const std::vector<std::size_t> &localDimensions,
                             const std::vector<std::size_t> &globalDimensions,
                             const std::vector<std::size_t> &globalOffsets,
                             const unsigned int skip,
                             const bool addType = false) const noexcept;

  /**
   * GetStats for primitive types except std::complex<T> types
   * @param variable
   * @return stats
   */
  template <class T>
  Stats<T> GetStats(const Variable<T> &variable) const noexcept
  {
    Stats<T> stats;
    const std::size_t valuesSize = variable.TotalSize();

    if (m_Verbosity == 0)
    {
      if (valuesSize >= 10000000) // ten million? this needs actual results
                                  // //here we can make decisions for threads
                                  // based on valuesSize
        GetMinMax(variable.m_AppValues, valuesSize, stats.Min, stats.Max,
                  m_Threads); // here we can add cores from constructor
      else
        GetMinMax(variable.m_AppValues, valuesSize, stats.Min, stats.Max);
    }
    return stats;
  }

  /**
   * GetStats for std::complex<T> types
   * @param variable
   * @return stats
   */
  template <class T>
  Stats<T> GetStats(const Variable<std::complex<T>> &variable) const noexcept
  {
    Stats<T> stats;
    const std::size_t valuesSize = variable.TotalSize();

    if (m_Verbosity == 0)
    {
      if (valuesSize >= 10000000) // ten million? this needs actual results
                                  // //here we can make decisions for threads
                                  // based on valuesSize
        GetMinMax(variable.m_AppValues, valuesSize, stats.Min, stats.Max,
                  m_Threads); // here we can add cores from constructor
      else
        GetMinMax(variable.m_AppValues, valuesSize, stats.Min, stats.Max);
    }
    return stats;
  }

  template <class T>
  void WriteBoundsRecord(const bool isScalar, const Stats<T> &stats,
                         std::vector<char> &buffer,
                         std::uint8_t &characteristicsCounter,
                         const bool addLength) const noexcept
  {
    if (isScalar == true)
    {
      WriteCharacteristicRecord(characteristic_value, stats.Min, buffer,
                                characteristicsCounter,
                                addLength); // stats.min = stats.max = value
      return;
    }

    if (m_Verbosity == 0) // default verbose
    {
      WriteCharacteristicRecord(characteristic_min, stats.Min, buffer,
                                characteristicsCounter, addLength);
      WriteCharacteristicRecord(characteristic_max, stats.Max, buffer,
                                characteristicsCounter, addLength);
    }
  }

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
                                 const bool addLength = false) const noexcept
  {
    const std::uint8_t id = characteristicID;
    CopyToBuffer(buffer, &id);

    if (addLength == true)
    {
      const std::uint16_t lengthOfCharacteristic = sizeof(T); // id
      CopyToBuffer(buffer, &lengthOfCharacteristic);
    }

    CopyToBuffer(buffer, &value);
    ++characteristicsCounter;
  }

  /**
   * Returns corresponding index of type BP1Index, if doesn't exists creates a
   * new one.
   * Used for variables and attributes
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
                       capsule::STLVector &buffer) const
      noexcept; ///< sets the metadata buffer in capsule with indices and
                /// minifooter
};

} // end namespace format
} // end namespace adios

#endif /* BP1WRITER_H_ */
