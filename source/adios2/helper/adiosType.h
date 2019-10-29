/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosType.h helper functions for types: conversion, mapping, etc.
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSTYPE_H_
#define ADIOS2_HELPER_ADIOSTYPE_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
/// \endcond

#include "adios2/common/ADIOSTypes.h"

namespace adios2
{
namespace helper
{
// TODO: deprecate
struct SubFileInfo
{
    /**  from characteristics, first = Start point, second =
    End point of block of data */
    Box<Dims> BlockBox;
    Box<Dims> IntersectionBox; ///< first = Start point, second = End point
    Box<size_t> Seeks;         ///< first = Start seek, second = End seek
};

/**
 * //TODO: deprecate Structure that contains the seek info per variable
 * <pre>
 *   key: subfile index
 *   value: (map)
 *     key: step
 *     value: (vector)
 *       index : block ID within step
 *       value : file seek box: first = seekStart, second = seekCount
 * </pre>
 */
using SubFileInfoMap =
    std::map<size_t, std::map<size_t, std::vector<SubFileInfo>>>;

struct BlockOperationInfo
{
    /** additional info: e.g. Type */
    Params Info;

    /** pre operation shape */
    Dims PreShape;
    /** pre operation start */
    Dims PreStart;
    /** pre operation count */
    Dims PreCount;

    /** payload offset position of operated data */
    size_t PayloadOffset = MaxSizeT;
    /** size in bytes **/
    size_t PayloadSize = MaxSizeT;
    /** Pre type sizeof */
    size_t PreSizeOf = 0;
};

/**
 * Contains SubStream info for intersecting block
 * key: subfile (substream) index
 * value : file seek box: first = seekStart, second = seekCount
 */
struct SubStreamBoxInfo
{
    /** stores information about any data operation applied in this block box */
    std::vector<BlockOperationInfo> OperationsInfo;

    /**  from characteristics, first = Start point, second =
     End point of block of data */
    Box<Dims> BlockBox;

    /** Intersection box between BlockBox and variable block
     *  first = Start point, second = End point */
    Box<Dims> IntersectionBox;

    /** Seeks (offsets) in serialized stream for intersection box */
    Box<size_t> Seeks;

    /** particular substream ID */
    size_t SubStreamID;

    bool ZeroBlock = false;
};

/**
 * Gets type from template parameter T
 * @return string with type
 */
template <class T>
std::string GetType() noexcept;

/**
 * Check in types set if "type" is one of the aliases for a certain type,
 * (e.g. if type = integer is an accepted alias for "int", returning true)
 * @param type input to be compared with an alias
 * @param aliases set containing aliases to a certain type, typically
 * Support::DatatypesAliases from Support.h
 * @return true: is an alias, false: is not
 */
template <class T>
bool IsTypeAlias(
    const std::string type,
    const std::map<std::string, std::set<std::string>> &aliases) noexcept;

/**
 * Converts a vector of dimensions to a CSV string
 * @param dims vector of dimensions
 * @return comma separate value (CSV)
 */
std::string DimsToCSV(const Dims &dimensions) noexcept;

/**
 * Converts comma-separated values (csv) to a vector of integers
 * @param csv "1,2,3"
 * @return vector<int> = { 1, 2, 3 }
 */
std::vector<int> CSVToVectorInt(const std::string csv) noexcept;

/**
 * Convert a vector of uint64_t element into size_t
 * @param in  uint64_t vector
 * @param out size_t vector with contents of in
 */
void ConvertUint64VectorToSizetVector(const std::vector<uint64_t> &in,
                                      std::vector<size_t> &out) noexcept;

/** Convert a C array of uint64_t elements to a vector of std::size_t elements
 *  @param number of elements
 *  @param input array of uint64_t elements
 *  @param vector of std::size_t elements. It will be resized to nElements.
 */
void Uint64ArrayToSizetVector(const size_t nElements, const uint64_t *in,
                              std::vector<size_t> &out) noexcept;

/** Convert a C array of uint64_t elements to a vector of std::size_t elements
 *  @param number of elements
 *  @param input array of uint64_t elements
 *  @return vector of std::size_t elements
 */
std::vector<size_t> Uint64ArrayToSizetVector(const size_t nElements,
                                             const uint64_t *in) noexcept;

/**
 * Converts a recognized time unit string to TimeUnit enum
 * @param timeUnit string with acceptable time unit
 * @param debugMode true: throw exception if timeUnitString not valid
 * @return TimeUnit enum (int) TimeUnit::s, TimeUnit::ms, etc.
 */
TimeUnit StringToTimeUnit(const std::string timeUnitString,
                          const bool debugMode, const std::string hint = "");

/**
 * Returns the conversion factor from input units Tb, Gb, Mb, Kb, to bytes as a
 * factor of 1024
 * @param units input
 * @param debugMode true: check if input units are valid
 * @return conversion factor to bytes, size_t
 */
size_t BytesFactor(const std::string units, const bool debugMode);

/**
 * Returns open mode as a string
 * @param openMode from ADIOSTypes.h
 * @param oneLetter if true returns a one letter version ("w", "a" or "r")
 * @return string with open mode
 */
std::string OpenModeToString(const Mode openMode,
                             const bool oneLetter = false) noexcept;

template <class T, class U>
std::vector<U> NewVectorType(const std::vector<T> &in);

template <class T, class U>
std::vector<U> NewVectorTypeFromArray(const T *in, const size_t inSize);

template <class T>
constexpr bool IsLvalue(T &&);

/**
 * Inquire for existing key and return value in a map
 * @param input contains key, value pairs
 * @return if found: a pointer to value, else a nullptr
 */
template <class T, class U>
U *InquireKey(const T &key, std::map<T, U> &input) noexcept;

/**
 * Inquire for existing key and return value in an unordered_map
 * @param input contains key, value pairs
 * @return if found: a pointer to value, else a nullptr
 */
template <class T, class U>
U *InquireKey(const T &key, std::unordered_map<T, U> &input) noexcept;

template <class T>
std::string VectorToCSV(const std::vector<T> &input) noexcept;

template <class T>
std::string ValueToString(const T value) noexcept;

/**
 * Checks if input pointer is nullptr ad throws an exception
 * @param pointer input to be checked against nullptr
 * @param hint additional exception information
 */
template <class T>
void CheckForNullptr(T *pointer, const std::string hint);

/**
 * Converts an unordered map key to a sorted set
 * @param hash input hash
 * @return ordered keys
 */
template <class T, class U>
std::set<T> KeysToSet(const std::unordered_map<T, U> &hash) noexcept;

/**
 * Calls map.erase(key) returning current value. Throws an exception if Key not
 * found.
 * @param key input
 * @param map input/output
 * @return value from found key
 */
template <class T, class U>
U EraseKey(const T &key, std::map<T, U> &map);

} // end namespace helper
} // end namespace adios2

#include "adiosType.inl"

#endif /* ADIOS2_HELPER_ADIOSTYPE_H_ */
