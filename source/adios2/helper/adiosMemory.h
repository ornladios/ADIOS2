/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.h : Memory copy operations functions using std::copy std::insert
 * and std::memcpy
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMEMORY_H_
#define ADIOS2_HELPER_ADIOSMEMORY_H_

/// \cond EXCLUDE_FROM_DOXYGEN
#include <string>
#include <vector>
/// \endcond

#include "adios2/ADIOSTypes.h"

namespace adios2
{
namespace helper
{

/**
 * Inserts source at the end of a buffer updating buffer.size()
 * @param buffer data destination calls insert()
 * @param source pointer to source data
 * @param elements number of elements of source type
 */
template <class T>
void InsertToBuffer(std::vector<char> &buffer, const T *source,
                    const size_t elements = 1) noexcept;

/**
 * Copies data to a specific location in the buffer updating position
 * Does not update vec.size().
 * @param buffer data destination used in std::copy
 * @param position starting position in buffer (in terms of T not bytes)
 * @param source pointer to source data
 * @param elements number of elements of source type
 */
template <class T>
void CopyToBuffer(std::vector<char> &buffer, size_t &position, const T *source,
                  const size_t elements = 1) noexcept;

/**
 * Copies data to a specific location in the buffer updating position using
 * threads.
 * Does not update vec.size().
 * @param buffer data destination used in std::copy
 * @param position starting position in buffer (in terms of T not bytes)
 * @param source pointer to source data
 * @param elements number of elements of source type
 * @param threads number of threads sharing the copy load
 */
template <class T>
void CopyToBufferThreads(std::vector<char> &buffer, size_t &position,
                         const T *source, const size_t elements = 1,
                         const unsigned int threads = 1) noexcept;

/**
 * Copy memory from a buffer at a certain input position
 * @param buffer data source
 * @param position start position to copy from buffer, modified to final
 * position
 * @param destination pointer to destination
 * @param elements  number of elements of destination type
 */
template <class T>
void CopyFromBuffer(const std::vector<char> &buffer, size_t &position,
                    T *destination, const size_t elements = 1) noexcept;

/**
 * Cast an element to uint64 and insert to a buffer
 * @param buffer data destination
 * @param element to be added to buffer
 */
template <class T>
void InsertU64(std::vector<char> &buffer, const T element) noexcept;

template <class T>
T ReadValue(const std::vector<char> &buffer, size_t &position) noexcept;

/**
 * Clips the contiguous memory corresponding to an intersection and puts it in
 * dest, where dest has a start and coun
 * @param dest to be populated (must be pre-allocated)
 * @param destStart offset selection for data pointer
 * @param destCount count selection for data pointer
 * @param contiguousMemory input contiguous memory
 * @param blockBox input block box for contiguous memory
 * @param intersectionBox between input blockBox and start,count box
 * @param isBlockMemory true: blockBox memory (operations), false: only
 * intersection
 * @param isRowMajor true: contiguous data is row major, false: column major
 * @param reverseDimensions true: data and contiguousMemory have different
 * ordering column/row or row/column major, respectively.
 */
template <class T>
void ClipContiguousMemory(T *dest, const Dims &destStart, const Dims &destCount,
                          const std::vector<char> &contiguousMemory,
                          const Box<Dims> &blockBox,
                          const Box<Dims> &intersectionBox,
                          const bool isRowMajor = true,
                          const bool reverseDimensions = false);

/**
 * Clips a vector returning the sub-vector between start and end (end is
 * exclusive), new size is end-start
 * @param vec
 * @param start
 * @param end
 */
template <class T>
void ClipVector(std::vector<T> &vec, const size_t start,
                const size_t end) noexcept;

template <class T>
void Resize(std::vector<T> &vec, const size_t dataSize, const bool debugMode,
            const std::string hint, T value = T());

/**
 * Author:Shawn Yang, shawnyang610@gmail.com
 * Copies n-dimensional Data from a source buffer to destination buffer, either
 * can be of any Major and Endianess. Return 1 if no overlap is found.
 * Copying from row-major to row-major of the same endian yields the best speed.
 * Optimizations: first: looks for the largest contiguous data
 * block size and copies the block of data as a whole. Second: by using
 * depth-first traversal for dynamic memory pointer calculations.
 * address calculation for each copied block is reduced to O(1) from O(n).
 * which means the computational cost is drastically reduced for data of higher
 * dimensions.
 * For copying involving column major, or different endianess only the
 * second optimization is applied.
 * Note: in case of super high dimensional data(over 10000 dimensions),
 * function stack may run out, set safeMode=true to switch to iterative
 * algms(a little slower due to explicit stack running less efficiently).
 * @param in pointer to source memory buffer
 * @param inStart source data starting offset
 * @param inCount source data structure
 * @param inIsRowMaj specifies major for input
 * @param inIsBigEndian specifies endianess for input
 * @param out pointer to destination memory buffer
 * @param outStart source data starting offset
 * @param outCount destination data structure
 * @param outIsRowMaj specifies major for output
 * @param outIsBigEndian specifies endianess for output
 * @param safeMode false:runs faster, the number of function stacks
 *                 used by recursive algm is equal to the number of dimensions.
 *                 true: runs a bit slower, same algorithm using the explicit
 *                 stack/simulated stack which has more overhead for the algm.
 */
template <class T>
int NdCopy(const char *in, const Dims &inStart, const Dims &inCount,
           const bool inIsRowMajor, const bool inIsLittleEndian, char *out,
           const Dims &outStart, const Dims &outCount, const bool outIsRowMajor,
           const bool outIsLittleEndian, const bool safeMode = false);

template <class T>
size_t PayloadSize(const T *data, const Dims &count) noexcept;

} // end namespace helper
} // end namespace adios2

#include "adiosMemory.inl"

#endif /* ADIOS2_HELPER_ADIOSMEMORY_H_ */
