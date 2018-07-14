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

template <class T>
void Resize(std::vector<T> &vec, const size_t dataSize, const bool debugMode,
            const std::string hint, T value = T());

/**
 * Author:Shawn Yang, shawnyang610@gmail.com
 * Copys n-dimensional Data from a source buffer to destination buffer, either
 * can be of any Major and Endianess. return 1 if no overlap is found.
 * Copying between same major and endianess yields the best speed. The
 * optimization is achived by, first: looks for the largest contiguous data
 * block size and copies the block of data as a whole.
 * second By using dynamic, depth-first traversal, The overhead for memory
 * address calculation for each copied block is reduced to O(1) from O(n).
 * which means copying speed is drastically improved for data of higher
 * dimensions.
 * For copying between buffers of diffenrent majors or endianesses, only the
 * second optimization is applied.
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
int NdCopy(const Buffer &in, const Dims &inStart, const Dims &inCount,
           bool inIsRowMaj, bool inIsBigEndian, Buffer &out,
           const Dims &outStart, const Dims &outCount, bool outIsRowMaj,
           bool outIsBigEndian, bool safeMode = false);

void NdCopyRecurDFSeqPadding(size_t curDim, char *&inOvlpBase,
                             char *&outOvlpBase, Dims &inOvlpGapSize,
                             Dims &outOvlpGapSize, Dims &ovlpCount,
                             size_t &minContDim, size_t &blockSize);

void NdCopyRecurDFSeqPaddingRevEndian(size_t curDim, char *&inOvlpBase,
                                      char *&outOvlpBase, Dims &inOvlpGapSize,
                                      Dims &outOvlpGapSize, Dims &ovlpCount,
                                      size_t minCountDim, size_t blockSize,
                                      size_t elmSize, size_t numElmsPerBlock);
void NdCopyRecurDFNonSeqDynamic(size_t curDim, char *inBase, char *outBase,
                                Dims &inRltvOvlpSPos, Dims &outRltvOvlpSPos,
                                Dims &inStride, Dims &outStride,
                                Dims &ovlpCount, size_t elmSize);
void NdCopyRecurDFNonSeqDynamicRevEndian(size_t curDim, char *inBase,
                                         char *outBase, Dims &inRltvOvlpSPos,
                                         Dims &outRltvOvlpSPos, Dims &inStride,
                                         Dims &outStride, Dims &ovlpCount,
                                         size_t elmSize);
void NdCopyIterDFSeqPadding(char *&inOvlpBase, char *&outOvlpBase,
                            Dims &inOvlpGapSize, Dims &outOvlpGapSize,
                            Dims &ovlpCount, size_t minContDim,
                            size_t blockSize);
void NdCopyIterDFSeqPaddingRevEndian(char *&inOvlpBase, char *&outOvlpBase,
                                     Dims &inOvlpGapSize, Dims &outOvlpGapSize,
                                     Dims &ovlpCount, size_t minContDim,
                                     size_t blockSize, size_t elmSize,
                                     size_t numElmsPerBlock);
void NdCopyIterDFDynamic(char *inBase, char *outBase, Dims &inRltvOvlpSPos,
                         Dims &outRltvOvlpSPos, Dims &inStride, Dims &outStride,
                         Dims &ovlpCount, size_t elmSize);
void NdCopyIterDFDynamicRevEndian(char *inBase, char *outBase,
                                  Dims &inRltvOvlpSPos, Dims &outRltvOvlpSPos,
                                  Dims &inStride, Dims &outStride,
                                  Dims &ovlpCount, size_t elmSize);

} // end namespace helper
} // end namespace adios2

#include "adiosMemory.inl"

#endif /* ADIOS2_HELPER_ADIOSMEMORY_H_ */
