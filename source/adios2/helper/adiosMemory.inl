/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.inl definition of template functions in adiosMemory.h
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_HELPER_ADIOSMEMORY_INL_
#define ADIOS2_HELPER_ADIOSMEMORY_INL_
#ifndef ADIOS2_HELPER_ADIOSMEMORY_H_
#error "Inline file should only be included from it's header, never on it's own"
#endif

/// \cond EXCLUDE_FROM_DOXYGEN
#include <algorithm> //std::copy, std::reverse_copy
#include <cstring>   //std::memcpy
#include <iostream>
#include <thread>
/// \endcond

#include "adios2/helper/adiosMath.h"
#include "adios2/helper/adiosSystem.h"
#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace helper
{

#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
template <class T>
inline void CopyEndianReverse(const char *src, const size_t payloadStride,
                              T *dest)
{
    if (sizeof(T) == 1)
    {
        std::copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
        return;
    }

    std::reverse_copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    std::reverse(dest, dest + payloadStride / sizeof(T));
}

template <>
inline void CopyEndianReverse<std::complex<float>>(const char *src,
                                                   const size_t payloadStride,
                                                   std::complex<float> *dest)
{
    std::reverse_copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    float *destF = reinterpret_cast<float *>(dest);
    std::reverse(destF, destF + payloadStride / sizeof(float));
}

template <>
inline void CopyEndianReverse<std::complex<double>>(const char *src,
                                                    const size_t payloadStride,
                                                    std::complex<double> *dest)
{
    std::reverse_copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    double *destF = reinterpret_cast<double *>(dest);
    std::reverse(destF, destF + payloadStride / sizeof(double));
}
#endif

template <class T>
void InsertToBuffer(std::vector<char> &buffer, const T *source,
                    const size_t elements) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    buffer.insert(buffer.end(), src, src + elements * sizeof(T));
}

#ifdef ADIOS2_HAVE_CUDA
template <class T>
void CopyFromGPUToBuffer(std::vector<char> &buffer, size_t &position,
                         const T *source, const size_t elements) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    MemcpyGPUToBuffer(buffer.data() + position, src, elements * sizeof(T));
    position += elements * sizeof(T);
}
#endif

template <class T>
void CopyToBuffer(std::vector<char> &buffer, size_t &position, const T *source,
                  const size_t elements) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    std::copy(src, src + elements * sizeof(T), buffer.begin() + position);
    position += elements * sizeof(T);
}

template <class T>
void CopyToBufferThreads(std::vector<char> &buffer, size_t &position,
                         const T *source, const size_t elements,
                         const unsigned int threads) noexcept
{
    if (elements == 0)
    {
        return;
    }

    if (threads == 1 || threads > elements)
    {
        CopyToBuffer(buffer, position, source, elements);
        return;
    }

    const size_t stride = elements / threads;    // elements per thread
    const size_t remainder = elements % threads; // remainder if not aligned
    const size_t last = stride + remainder;

    std::vector<std::thread> copyThreads;
    copyThreads.reserve(threads);

    const char *src = reinterpret_cast<const char *>(source);

    for (unsigned int t = 0; t < threads; ++t)
    {
        const size_t bufferStart = position + stride * t * sizeof(T);
        const size_t srcStart = stride * t * sizeof(T);
        if (t == threads - 1) // last thread takes stride + remainder
        {
            copyThreads.push_back(std::thread(std::memcpy, &buffer[bufferStart],
                                              &src[srcStart],
                                              last * sizeof(T)));
            // std::copy not working properly with std::thread...why?
            //            copyThreads.push_back(std::thread(std::copy,
            //            &src[srcStart],
            //                                              &src[srcStart] +
            //                                              last * sizeof(T),
            //                                              buffer.begin() +
            //                                              bufferStart));
        }
        else
        {
            copyThreads.push_back(std::thread(std::memcpy, &buffer[bufferStart],
                                              &src[srcStart],
                                              stride * sizeof(T)));
            // std::copy not working properly with std::thread...why?
            //            copyThreads.push_back(std::thread(
            //                std::copy, &src[srcStart], &src[srcStart] + stride
            //                * sizeof(T),
            //                buffer.begin() + bufferStart));
        }
    }

    for (auto &copyThread : copyThreads)
    {
        copyThread.join();
    }

    position += elements * sizeof(T);
}

template <class T>
inline void ReverseCopyFromBuffer(const std::vector<char> &buffer,
                                  size_t &position, T *destination,
                                  const size_t elements) noexcept
{
    std::reverse_copy(buffer.begin() + position,
                      buffer.begin() + position + sizeof(T) * elements,
                      reinterpret_cast<char *>(destination));
    position += elements * sizeof(T);
}

template <class T>
void CopyFromBuffer(const std::vector<char> &buffer, size_t &position,
                    T *destination, size_t elements) noexcept
{
    std::copy(buffer.begin() + position,
              buffer.begin() + position + sizeof(T) * elements,
              reinterpret_cast<char *>(destination));
    position += elements * sizeof(T);
}

template <class T>
void InsertU64(std::vector<char> &buffer, const T element) noexcept
{
    const uint64_t element64 = static_cast<uint64_t>(element);
    InsertToBuffer(buffer, &element64, 1);
}

template <class T>
inline T ReadValue(const std::vector<char> &buffer, size_t &position,
                   const bool isLittleEndian) noexcept
{
    T value;

#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (IsLittleEndian() != isLittleEndian)
    {
        ReverseCopyFromBuffer(buffer, position, &value);
    }
    else
    {
        CopyFromBuffer(buffer, position, &value);
    }
#else
    CopyFromBuffer(buffer, position, &value);
#endif
    return value;
}

template <>
inline std::complex<float>
ReadValue<std::complex<float>>(const std::vector<char> &buffer,
                               size_t &position,
                               const bool isLittleEndian) noexcept
{
    std::complex<float> value;

#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (IsLittleEndian() != isLittleEndian)
    {
        ReverseCopyFromBuffer(buffer, position, &value);
        return std::complex<float>(value.imag(), value.real());
    }
    else
    {
        CopyFromBuffer(buffer, position, &value);
    }
#else
    CopyFromBuffer(buffer, position, &value);
#endif
    return value;
}

template <>
inline std::complex<double>
ReadValue<std::complex<double>>(const std::vector<char> &buffer,
                                size_t &position,
                                const bool isLittleEndian) noexcept
{
    std::complex<double> value;

#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (IsLittleEndian() != isLittleEndian)
    {
        ReverseCopyFromBuffer(buffer, position, &value);
        return std::complex<double>(value.imag(), value.real());
    }
    else
    {
        CopyFromBuffer(buffer, position, &value);
    }
#else
    CopyFromBuffer(buffer, position, &value);
#endif
    return value;
}

template <class T>
inline void ReadArray(const std::vector<char> &buffer, size_t &position,
                      T *output, const size_t nElems,
                      const bool isLittleEndian) noexcept
{
#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (IsLittleEndian() != isLittleEndian)
    {
        ReverseCopyFromBuffer(buffer, position, output, nElems);
    }
    else
    {
        CopyFromBuffer(buffer, position, output, nElems);
    }
#else
    CopyFromBuffer(buffer, position, output, nElems);
#endif
}

template <>
inline void ReadArray<std::complex<float>>(const std::vector<char> &buffer,
                                           size_t &position,
                                           std::complex<float> *output,
                                           const size_t nElems,
                                           const bool isLittleEndian) noexcept
{
#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (IsLittleEndian() != isLittleEndian)
    {
        ReverseCopyFromBuffer(buffer, position, output, nElems);
    }
    else
    {
        CopyFromBuffer(buffer, position, output, nElems);
    }
#else
    CopyFromBuffer(buffer, position, output, nElems);
#endif
}

template <>
inline void ReadArray<std::complex<double>>(const std::vector<char> &buffer,
                                            size_t &position,
                                            std::complex<double> *output,
                                            const size_t nElems,
                                            const bool isLittleEndian) noexcept
{
#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (IsLittleEndian() != isLittleEndian)
    {
        ReverseCopyFromBuffer(buffer, position, output, nElems);
    }
    else
    {
        CopyFromBuffer(buffer, position, output, nElems);
    }
#else
    CopyFromBuffer(buffer, position, output, nElems);
#endif
}

template <class T>
void ClipVector(std::vector<T> &vec, const size_t start,
                const size_t end) noexcept
{
    vec.resize(end);
    vec.erase(vec.begin(), vec.begin() + start);
}

template <class T, class U>
void CopyMemoryBlock(T *dest, const Dims &destStart, const Dims &destCount,
                     const bool destRowMajor, const U *src,
                     const Dims &srcStart, const Dims &srcCount,
                     const bool srcRowMajor, const bool endianReverse,
                     const Dims &destMemStart, const Dims &destMemCount,
                     const Dims &srcMemStart, const Dims &srcMemCount) noexcept
{
    // transform everything to payload dims
    const Dims destStartPayload = PayloadDims<T>(destStart, destRowMajor);
    const Dims destCountPayload = PayloadDims<T>(destCount, destRowMajor);
    const Dims destMemStartPayload = PayloadDims<T>(destMemStart, destRowMajor);
    const Dims destMemCountPayload = PayloadDims<T>(destMemCount, destRowMajor);

    const Dims srcStartPayload = PayloadDims<U>(srcStart, srcRowMajor);
    const Dims srcCountPayload = PayloadDims<U>(srcCount, srcRowMajor);
    const Dims srcMemStartPayload = PayloadDims<U>(srcMemStart, srcRowMajor);
    const Dims srcMemCountPayload = PayloadDims<U>(srcMemCount, srcRowMajor);

    CopyPayload(reinterpret_cast<char *>(dest), destStartPayload,
                destCountPayload, destRowMajor,
                reinterpret_cast<const char *>(src), srcStartPayload,
                srcCountPayload, srcRowMajor, destMemStartPayload,
                destMemCountPayload, srcMemStartPayload, srcMemCountPayload,
                endianReverse, GetDataType<T>());
}

template <class T>
void ClipContiguousMemory(T *dest, const Dims &destStart, const Dims &destCount,
                          const char *contiguousMemory,
                          const Box<Dims> &blockBox,
                          const Box<Dims> &intersectionBox,
                          const bool isRowMajor, const bool reverseDimensions,
                          const bool endianReverse)
{
    auto lf_ClipRowMajor =
        [](T *dest, const Dims &destStart, const Dims &destCount,
           const char *contiguousMemory, const Box<Dims> &blockBox,
           const Box<Dims> &intersectionBox, const bool isRowMajor,
           const bool reverseDimensions, const bool endianReverse)

    {
        const Dims &istart = intersectionBox.first;
        const Dims &iend = intersectionBox.second;

        Dims currentPoint(istart); // current point for memory copy
        // convert selection to EndBox and reverse if we are inside a
        // column-major reader
        const Box<Dims> selectionBox =
            helper::StartEndBox(destStart, destCount, reverseDimensions);

        const size_t dimensions = istart.size();

        /* Determine how many dimensions we can copy at once.
           nContDim = dimensions: single contiguous copy
           ncontDim = 2: a 2D slice
           nContDim = 1: line by line
        */
        size_t nContDim = 1;
        while (nContDim <= dimensions - 1 &&
               blockBox.first[dimensions - nContDim] ==
                   istart[dimensions - nContDim] &&
               blockBox.second[dimensions - nContDim] ==
                   iend[dimensions - nContDim] &&
               blockBox.first[dimensions - nContDim] ==
                   selectionBox.first[dimensions - nContDim] &&
               blockBox.second[dimensions - nContDim] ==
                   selectionBox.second[dimensions - nContDim])
        {
            ++nContDim;
        }
        // Note: 1 <= nContDim <= dimensions
        size_t nContElems = 1;
        for (size_t i = 1; i <= nContDim; ++i)
        {
            nContElems *= (iend[dimensions - i] - istart[dimensions - i] + 1);
        }

        const size_t stride = nContElems * sizeof(T);

        const size_t intersectionStart =
            helper::LinearIndex(blockBox, intersectionBox.first, true) *
            sizeof(T);

        bool run = true;
        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t contiguousStart =
                helper::LinearIndex(blockBox, currentPoint, true) * sizeof(T) -
                intersectionStart;

            const size_t variableStart =
                helper::LinearIndex(selectionBox, currentPoint, true);

            CopyContiguousMemory(contiguousMemory + contiguousStart, stride,
                                 dest + variableStart, endianReverse);

            // Here update each non-contiguous dim recursively
            if (nContDim >= dimensions)
            {
                run = false; // we copied everything at once
            }
            else
            {
                size_t p = dimensions - nContDim - 1;
                while (true)
                {
                    ++currentPoint[p];
                    if (currentPoint[p] > iend[p])
                    {
                        if (p == 0)
                        {
                            run = false; // we are done
                            break;
                        }
                        else
                        {
                            currentPoint[p] = istart[p];
                            --p;
                        }
                    }
                    else
                    {
                        break; // break inner p loop
                    }
                } // dimension index update
            }
        } // run
    };

    auto lf_ClipColumnMajor =
        [](T *dest, const Dims &destStart, const Dims &destCount,
           const char *contiguousMemory, const Box<Dims> &blockBox,
           const Box<Dims> &intersectionBox, const bool isRowMajor,
           const bool reverseDimensions, const bool endianReverse)

    {
        const Dims &istart = intersectionBox.first;
        const Dims &iend = intersectionBox.second;

        Dims currentPoint(istart); // current point for memory copy

        const Box<Dims> selectionBox =
            helper::StartEndBox(destStart, destCount, reverseDimensions);

        const size_t dimensions = istart.size();
        /* Determine how many dimensions we can copy at once.
           nContDim = dimensions: single contiguous copy
           ncontDim = 2: a 2D slice
           nContDim = 1: line by line
        */
        size_t nContDim = 1;
        while (
            nContDim <= dimensions - 1 &&
            blockBox.first[nContDim - 1] == istart[nContDim - 1] &&
            blockBox.second[nContDim - 1] == iend[nContDim - 1] &&
            blockBox.first[nContDim - 1] == selectionBox.first[nContDim - 1] &&
            blockBox.second[nContDim - 1] == selectionBox.second[nContDim - 1])
        {
            ++nContDim;
        }
        // Note: 1 <= nContDim <= dimensions
        size_t nContElems = 1;
        for (size_t i = 0; i < nContDim; ++i)
        {
            nContElems *= (iend[i] - istart[i] + 1);
        }

        const size_t stride = nContElems * sizeof(T);

        const size_t intersectionStart =
            helper::LinearIndex(blockBox, intersectionBox.first, false) *
            sizeof(T);

        bool run = true;
        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t contiguousStart =
                helper::LinearIndex(blockBox, currentPoint, false) * sizeof(T) -
                intersectionStart;

            const size_t variableStart =
                helper::LinearIndex(selectionBox, currentPoint, false);

            CopyContiguousMemory(contiguousMemory + contiguousStart, stride,
                                 dest + variableStart, endianReverse);

            // Here update each non-contiguous dim recursively.
            if (nContDim >= dimensions)
            {
                run = false; // we copied everything at once
            }
            else
            {
                size_t p = nContDim;
                while (true)
                {
                    ++currentPoint[p];
                    if (currentPoint[p] > iend[p])
                    {
                        if (p == dimensions - 1)
                        {
                            run = false; // we are done
                            break;
                        }
                        else
                        {
                            currentPoint[p] = istart[p];
                            ++p;
                        }
                    }
                    else
                    {
                        break; // break inner p loop
                    }
                } // dimension index update
            }
        }
    };

    const Dims &start = intersectionBox.first;
    if (start.size() == 1) // 1D copy memory
    {
        const size_t normalizedStart = start.front() - destStart.front();

        const Dims &start = intersectionBox.first;
        const Dims &end = intersectionBox.second;
        const size_t stride = (end.back() - start.back() + 1) * sizeof(T);

        CopyContiguousMemory(contiguousMemory, stride, dest + normalizedStart,
                             endianReverse);
        return;
    }

    if (isRowMajor) // stored with C, C++, Python
    {
        lf_ClipRowMajor(dest, destStart, destCount, contiguousMemory, blockBox,
                        intersectionBox, isRowMajor, reverseDimensions,
                        endianReverse);
    }
    else // stored with Fortran, R
    {
        lf_ClipColumnMajor(dest, destStart, destCount, contiguousMemory,
                           blockBox, intersectionBox, isRowMajor,
                           reverseDimensions, endianReverse);
    }
}

template <class T>
void ClipContiguousMemory(T *dest, const Dims &destStart, const Dims &destCount,
                          const std::vector<char> &contiguousMemory,
                          const Box<Dims> &blockBox,
                          const Box<Dims> &intersectionBox,
                          const bool isRowMajor, const bool reverseDimensions,
                          const bool endianReverse)
{

    ClipContiguousMemory(dest, destStart, destCount, contiguousMemory.data(),
                         blockBox, intersectionBox, isRowMajor,
                         reverseDimensions, endianReverse);
}

template <class T>
void CopyContiguousMemory(const char *src, const size_t payloadStride, T *dest,
                          const bool endianReverse)
{
#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (endianReverse)
    {
        CopyEndianReverse<T>(src, payloadStride, dest);
    }
    else
    {
        std::copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
    }
#else
    std::copy(src, src + payloadStride, reinterpret_cast<char *>(dest));
#endif
}

template <class T>
void Resize(std::vector<T> &vec, const size_t dataSize, const std::string hint,
            T value)
{
    try
    {
        // avoid power of 2 capacity growth
        vec.reserve(dataSize);
        vec.resize(dataSize, value);
    }
    catch (...)
    {
        std::throw_with_nested(std::runtime_error(
            "ERROR: buffer overflow when resizing to " +
            std::to_string(dataSize) + " bytes, " + hint + "\n"));
    }
}

//***************Start of NdCopy() and its 8 helpers ***************
// Author:Shawn Yang, shawnyang610@gmail.com
//
// NdCopyRecurDFSeqPadding(): helper function
// Copys n-dimensional Data from input to output in row major and
// same endianess.
// It looks for the largest contiguous data block size in the overlap (by its
// helper
// functions) and copies to the output buffer in blocks. the memory address
// calculation complexity for copying each block is minimized to O(1), which is
// independent of the number of dimensions.
static inline void
NdCopyRecurDFSeqPadding(size_t curDim, const char *&inOvlpBase,
                        char *&outOvlpBase, Dims &inOvlpGapSize,
                        Dims &outOvlpGapSize, Dims &ovlpCount,
                        size_t &minContDim, size_t &blockSize)
{
    // note: all elements in and below this node are contiguous on input and
    // output
    // copy the contiguous data block
    if (curDim == minContDim)
    {
        std::memcpy(outOvlpBase, inOvlpBase, blockSize);
        inOvlpBase += blockSize + inOvlpGapSize[curDim];
        outOvlpBase += blockSize + outOvlpGapSize[curDim];
    }
    // recursively call itself in order, for every element current node has
    // on a deeper level, stops upon reaching minContDim
    // case: curDim<minCountDim
    else
    {
        for (size_t i = 0; i < ovlpCount[curDim]; i++)
        {
            NdCopyRecurDFSeqPadding(curDim + 1, inOvlpBase, outOvlpBase,
                                    inOvlpGapSize, outOvlpGapSize, ovlpCount,
                                    minContDim, blockSize);
        }
        // the gap between current node and the next needs to be padded so that
        // next contigous block starts at the correct position for both input
        // and output
        // the size of the gap depends on the depth in dimensions,level
        // backtracked and
        // the difference in element counts between the Input/output and overlap
        // area.
        inOvlpBase += inOvlpGapSize[curDim];
        outOvlpBase += outOvlpGapSize[curDim];
    }
}

// NdCopyRecurDFSeqPaddingRevEndian(): helper function
// Copys n-dimensional Data from input to output in the row major but in
// reversed endianess. the memory address calculation complexity for copying
// each element is minimized to average O(1), which is independent of
// the number of dimensions.

static inline void
NdCopyRecurDFSeqPaddingRevEndian(size_t curDim, const char *&inOvlpBase,
                                 char *&outOvlpBase, Dims &inOvlpGapSize,
                                 Dims &outOvlpGapSize, Dims &ovlpCount,
                                 size_t minCountDim, size_t blockSize,
                                 size_t elmSize, size_t numElmsPerBlock)
{
    if (curDim == minCountDim)
    {
        // each byte of each element in the continuous block needs
        // to be copied in reverse order
        for (size_t i = 0; i < numElmsPerBlock; i++)
        {
            for (size_t j = 0; j < elmSize; j++)
            {
                outOvlpBase[j] = inOvlpBase[elmSize - 1 - j];
            }
            inOvlpBase += elmSize;
            outOvlpBase += elmSize;
        }
    }
    // case: curDim<minCountDim
    else
    {
        for (size_t i = 0; i < ovlpCount[curDim]; i++)
        {
            NdCopyRecurDFSeqPaddingRevEndian(
                curDim + 1, inOvlpBase, outOvlpBase, inOvlpGapSize,
                outOvlpGapSize, ovlpCount, minCountDim, blockSize, elmSize,
                numElmsPerBlock);
        }
    }
    inOvlpBase += inOvlpGapSize[curDim];
    outOvlpBase += outOvlpGapSize[curDim];
}

// NdCopyRecurDFNonSeqDynamic(): helper function
// Copys n-dimensional Data from input to output in the same Endianess
// used for buffer of Column major
// the memory address calculation complexity for copying each element is
// minimized to average O(1), which is independent of the number of dimensions.
static inline void NdCopyRecurDFNonSeqDynamic(size_t curDim, const char *inBase,
                                              char *outBase,
                                              Dims &inRltvOvlpSPos,
                                              Dims &outRltvOvlpSPos,
                                              Dims &inStride, Dims &outStride,
                                              Dims &ovlpCount, size_t elmSize)
{
    if (curDim == inStride.size())
    {
        std::memcpy(outBase, inBase, elmSize);
    }
    else
    {
        for (size_t i = 0; i < ovlpCount[curDim]; i++)
        {
            NdCopyRecurDFNonSeqDynamic(
                curDim + 1,
                inBase + (inRltvOvlpSPos[curDim] + i) * inStride[curDim],
                outBase + (outRltvOvlpSPos[curDim] + i) * outStride[curDim],
                inRltvOvlpSPos, outRltvOvlpSPos, inStride, outStride, ovlpCount,
                elmSize);
        }
    }
}

// NdCopyRecurDFNonSeqDynamicRevEndian(): helper function
// Copies n-dimensional Data from input to output in the reversed Endianess and
// Major.
// The memory address calculation complexity for copying each element is
// minimized to average O(1), which is independent of the number of dimensions.

static inline void NdCopyRecurDFNonSeqDynamicRevEndian(
    size_t curDim, const char *inBase, char *outBase, Dims &inRltvOvlpSPos,
    Dims &outRltvOvlpSPos, Dims &inStride, Dims &outStride, Dims &ovlpCount,
    size_t elmSize)
{
    if (curDim == inStride.size())
    {
        for (size_t i = 0; i < elmSize; i++)
        {
            outBase[i] = inBase[elmSize - 1 - i];
        }
    }
    else
    {
        for (size_t i = 0; i < ovlpCount[curDim]; i++)
        {
            NdCopyRecurDFNonSeqDynamicRevEndian(
                curDim + 1,
                inBase + (inRltvOvlpSPos[curDim] + i) * inStride[curDim],
                outBase + (outRltvOvlpSPos[curDim] + i) * outStride[curDim],
                inRltvOvlpSPos, outRltvOvlpSPos, inStride, outStride, ovlpCount,
                elmSize);
        }
    }
}

static inline void NdCopyIterDFSeqPadding(const char *&inOvlpBase,
                                          char *&outOvlpBase,
                                          Dims &inOvlpGapSize,
                                          Dims &outOvlpGapSize, Dims &ovlpCount,
                                          size_t minContDim, size_t blockSize)
{
    Dims pos(ovlpCount.size(), 0);
    size_t curDim = 0;
    while (true)
    {
        while (curDim != minContDim)
        {
            pos[curDim]++;
            curDim++;
        }
        std::memcpy(outOvlpBase, inOvlpBase, blockSize);
        inOvlpBase += blockSize;
        outOvlpBase += blockSize;
        do
        {
            if (curDim == 0)
            {
                return;
            }
            inOvlpBase += inOvlpGapSize[curDim];
            outOvlpBase += outOvlpGapSize[curDim];
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}

static inline void NdCopyIterDFSeqPaddingRevEndian(
    const char *&inOvlpBase, char *&outOvlpBase, Dims &inOvlpGapSize,
    Dims &outOvlpGapSize, Dims &ovlpCount, size_t minContDim, size_t blockSize,
    size_t elmSize, size_t numElmsPerBlock)
{
    Dims pos(ovlpCount.size(), 0);
    size_t curDim = 0;
    while (true)
    {
        while (curDim != minContDim)
        {
            pos[curDim]++;
            curDim++;
        }
        for (size_t i = 0; i < numElmsPerBlock; i++)
        {
            for (size_t j = 0; j < elmSize; j++)
            {
                outOvlpBase[j] = inOvlpBase[elmSize - 1 - j];
            }
            inOvlpBase += elmSize;
            outOvlpBase += elmSize;
        }
        do
        {
            if (curDim == 0)
            {
                return;
            }
            inOvlpBase += inOvlpGapSize[curDim];
            outOvlpBase += outOvlpGapSize[curDim];
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}
static inline void NdCopyIterDFDynamic(const char *inBase, char *outBase,
                                       Dims &inRltvOvlpSPos,
                                       Dims &outRltvOvlpSPos, Dims &inStride,
                                       Dims &outStride, Dims &ovlpCount,
                                       size_t elmSize)
{
    size_t curDim = 0;
    Dims pos(ovlpCount.size() + 1, 0);
    std::vector<const char *> inAddr(ovlpCount.size() + 1);
    inAddr[0] = inBase;
    std::vector<char *> outAddr(ovlpCount.size() + 1);
    outAddr[0] = outBase;
    while (true)
    {
        while (curDim != inStride.size())
        {
            inAddr[curDim + 1] =
                inAddr[curDim] +
                (inRltvOvlpSPos[curDim] + pos[curDim]) * inStride[curDim];
            outAddr[curDim + 1] =
                outAddr[curDim] +
                (outRltvOvlpSPos[curDim] + pos[curDim]) * outStride[curDim];
            pos[curDim]++;
            curDim++;
        }
        std::memcpy(outAddr[curDim], inAddr[curDim], elmSize);
        do
        {
            if (curDim == 0)
            {
                return;
            }
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}

static inline void NdCopyIterDFDynamicRevEndian(const char *inBase,
                                                char *outBase,
                                                Dims &inRltvOvlpSPos,
                                                Dims &outRltvOvlpSPos,
                                                Dims &inStride, Dims &outStride,
                                                Dims &ovlpCount, size_t elmSize)
{
    size_t curDim = 0;
    Dims pos(ovlpCount.size() + 1, 0);
    std::vector<const char *> inAddr(ovlpCount.size() + 1);
    inAddr[0] = inBase;
    std::vector<char *> outAddr(ovlpCount.size() + 1);
    outAddr[0] = outBase;
    while (true)
    {
        while (curDim != inStride.size())
        {
            inAddr[curDim + 1] =
                inAddr[curDim] +
                (inRltvOvlpSPos[curDim] + pos[curDim]) * inStride[curDim];
            outAddr[curDim + 1] =
                outAddr[curDim] +
                (outRltvOvlpSPos[curDim] + pos[curDim]) * outStride[curDim];
            pos[curDim]++;
            curDim++;
        }
        for (size_t i = 0; i < elmSize; i++)
        {
            outAddr[curDim][i] = inAddr[curDim][elmSize - 1 - i];
        }
        do
        {
            if (curDim == 0)
            {
                return;
            }
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}

template <class T>
int NdCopy(const char *in, const Dims &inStart, const Dims &inCount,
           const bool inIsRowMajor, const bool inIsLittleEndian, char *out,
           const Dims &outStart, const Dims &outCount, const bool outIsRowMajor,
           const bool outIsLittleEndian, const Dims &inMemStart,
           const Dims &inMemCount, const Dims &outMemStart,
           const Dims &outMemCount, const bool safeMode)

{

    // use values of ioStart and ioCount if ioMemStart and ioMemCount are
    // left as default
    Dims inMemStartNC = inMemStart.empty() ? inStart : inMemStart;
    Dims inMemCountNC = inMemCount.empty() ? inCount : inMemCount;
    Dims outMemStartNC = outMemStart.empty() ? outStart : outMemStart;
    Dims outMemCountNC = outMemCount.empty() ? outCount : outMemCount;

    Dims inEnd(inStart.size());
    Dims outEnd(inStart.size());
    Dims ovlpStart(inStart.size());
    Dims ovlpEnd(inStart.size());
    Dims ovlpCount(inStart.size());
    Dims inStride(inStart.size());
    Dims outStride(inStart.size());
    Dims inOvlpGapSize(inStart.size());
    Dims outOvlpGapSize(inStart.size());
    Dims inRltvOvlpStartPos(inStart.size());
    Dims outRltvOvlpStartPos(inStart.size());
    size_t minContDim, blockSize;
    const char *inOvlpBase = nullptr;
    char *outOvlpBase = nullptr;
    auto GetInEnd = [](Dims &inEnd, const Dims &inStart, const Dims &inCount) {
        for (size_t i = 0; i < inStart.size(); i++)
        {
            inEnd[i] = inStart[i] + inCount[i] - 1;
        }
    };
    auto GetOutEnd = [](Dims &outEnd, const Dims &outStart,
                        const Dims &output_count) {
        for (size_t i = 0; i < outStart.size(); i++)
        {
            outEnd[i] = outStart[i] + output_count[i] - 1;
        }
    };
    auto GetOvlpStart = [](Dims &ovlpStart, const Dims &inStart,
                           const Dims &outStart) {
        for (size_t i = 0; i < ovlpStart.size(); i++)
        {
            ovlpStart[i] = inStart[i] > outStart[i] ? inStart[i] : outStart[i];
        }
    };
    auto GetOvlpEnd = [](Dims &ovlpEnd, Dims &inEnd, Dims &outEnd) {
        for (size_t i = 0; i < ovlpEnd.size(); i++)
        {
            ovlpEnd[i] = inEnd[i] < outEnd[i] ? inEnd[i] : outEnd[i];
        }
    };
    auto GetOvlpCount = [](Dims &ovlpCount, Dims &ovlpStart, Dims &ovlpEnd) {
        for (size_t i = 0; i < ovlpCount.size(); i++)
        {
            ovlpCount[i] = ovlpEnd[i] - ovlpStart[i] + 1;
        }
    };
    auto HasOvlp = [](Dims &ovlpStart, Dims &ovlpEnd) {
        for (size_t i = 0; i < ovlpStart.size(); i++)
        {
            if (ovlpEnd[i] < ovlpStart[i])
            {
                return false;
            }
        }
        return true;
    };

    auto GetIoStrides = [](Dims &ioStride, const Dims &ioCount,
                           size_t elmSize) {
        // ioStride[i] holds the total number of elements under each element
        // of the i'th dimension
        ioStride[ioStride.size() - 1] = elmSize;
        if (ioStride.size() > 1)
        {
            ioStride[ioStride.size() - 2] =
                ioCount[ioStride.size() - 1] * elmSize;
        }
        if (ioStride.size() > 2)
        {
            size_t i = ioStride.size() - 3;
            while (true)
            {
                ioStride[i] = ioCount[i + 1] * ioStride[i + 1];
                if (i == 0)
                {
                    break;
                }
                else
                {
                    i--;
                }
            }
        }
    };

    auto GetInOvlpBase = [](const char *&inOvlpBase, const char *in,
                            const Dims &inStart, Dims &inStride,
                            Dims &ovlpStart) {
        inOvlpBase = in;
        for (size_t i = 0; i < inStart.size(); i++)
        {
            inOvlpBase = inOvlpBase + (ovlpStart[i] - inStart[i]) * inStride[i];
        }
    };
    auto GetOutOvlpBase = [](char *&outOvlpBase, char *out,
                             const Dims &outStart, Dims &outStride,
                             Dims &ovlpStart) {
        outOvlpBase = out;
        for (size_t i = 0; i < outStart.size(); i++)
        {
            outOvlpBase =
                outOvlpBase + (ovlpStart[i] - outStart[i]) * outStride[i];
        }
    };
    auto GetIoOvlpGapSize = [](Dims &ioOvlpGapSize, Dims &ioStride,
                               const Dims &ioCount, Dims &ovlpCount) {
        for (size_t i = 0; i < ioOvlpGapSize.size(); i++)
        {
            ioOvlpGapSize[i] = (ioCount[i] - ovlpCount[i]) * ioStride[i];
        }
    };
    auto GetMinContDim = [](const Dims &inCount, const Dims outCount,
                            Dims &ovlpCount) {
        //    note: minContDim is the first index where its input box and
        //    overlap box
        //    are not fully match. therefore all data below this branch is
        //    contingous
        //    and this determins the Biggest continuous block size - Each
        //    element of the
        //    current dimension.
        size_t i = ovlpCount.size() - 1;
        while (true)
        {
            if (i == 0)
            {
                break;
            }
            if ((inCount[i] != ovlpCount[i]) || (outCount[i] != ovlpCount[i]))
            {
                break;
            }
            i--;
        }
        return i;
    };
    auto GetBlockSize = [](Dims &ovlpCount, size_t minContDim, size_t elmSize) {
        size_t res = elmSize;
        for (size_t i = minContDim; i < ovlpCount.size(); i++)
        {
            res *= ovlpCount[i];
        }
        return res;
    };

    auto GetRltvOvlpStartPos = [](Dims &ioRltvOvlpStart, const Dims &ioStart,
                                  Dims &ovlpStart) {
        for (size_t i = 0; i < ioStart.size(); i++)
        {
            ioRltvOvlpStart[i] = ovlpStart[i] - ioStart[i];
        }
    };

    // main flow
    // row-major ==> row-major mode
    // algrithm optimizations:
    // 1. contigous data copying
    // 2. mem pointer arithmetics by sequential padding. O(1) overhead/block
    if (inIsRowMajor && outIsRowMajor)
    {
        GetInEnd(inEnd, inStart, inCount);
        GetOutEnd(outEnd, outStart, outCount);
        GetOvlpStart(ovlpStart, inStart, outStart);
        GetOvlpEnd(ovlpEnd, inEnd, outEnd);
        GetOvlpCount(ovlpCount, ovlpStart, ovlpEnd);
        if (!HasOvlp(ovlpStart, ovlpEnd))
        {
            return 1; // no overlap found
        }
        GetIoStrides(inStride, inMemCountNC, sizeof(T));
        GetIoStrides(outStride, outMemCountNC, sizeof(T));
        GetIoOvlpGapSize(inOvlpGapSize, inStride, inMemCountNC, ovlpCount);
        GetIoOvlpGapSize(outOvlpGapSize, outStride, outMemCountNC, ovlpCount);
        GetInOvlpBase(inOvlpBase, in, inMemStartNC, inStride, ovlpStart);
        GetOutOvlpBase(outOvlpBase, out, outMemStartNC, outStride, ovlpStart);
        minContDim = GetMinContDim(inMemCountNC, outMemCountNC, ovlpCount);
        blockSize = GetBlockSize(ovlpCount, minContDim, sizeof(T));
        // same endianess mode: most optimized, contiguous data copying
        // algorithm used.
        if (inIsLittleEndian == outIsLittleEndian)
        {
            // most efficient algm
            // warning: number of function stacks used is number of dimensions
            // of data.
            if (!safeMode)
            {
                NdCopyRecurDFSeqPadding(0, inOvlpBase, outOvlpBase,
                                        inOvlpGapSize, outOvlpGapSize,
                                        ovlpCount, minContDim, blockSize);
            }
            else // safeMode
            {
                //      //alternative iterative version, 10% slower then
                //      recursive
                //      //use it when very high demension is used.
                NdCopyIterDFSeqPadding(inOvlpBase, outOvlpBase, inOvlpGapSize,
                                       outOvlpGapSize, ovlpCount, minContDim,
                                       blockSize);
            }
        }
        // different endianess mode
        else
        {
            if (!safeMode)
            {
                NdCopyRecurDFSeqPaddingRevEndian(
                    0, inOvlpBase, outOvlpBase, inOvlpGapSize, outOvlpGapSize,
                    ovlpCount, minContDim, blockSize, sizeof(T),
                    blockSize / sizeof(T));
            }
            else
            {
                NdCopyIterDFSeqPaddingRevEndian(
                    inOvlpBase, outOvlpBase, inOvlpGapSize, outOvlpGapSize,
                    ovlpCount, minContDim, blockSize, sizeof(T),
                    blockSize / sizeof(T));
            }
        }
    }

    // Copying modes involing col-major
    // algorithm optimization:
    // 1. mem ptr arithmetics: O(1) overhead per block, dynamic/non-sequential
    // padding
    else
    {
        //        Dims revInCount(inCount);
        //        Dims revOutCount(outCount);
        //
        // col-major ==> col-major mode
        if (!inIsRowMajor && !outIsRowMajor)
        {

            GetInEnd(inEnd, inStart, inCount);
            GetOutEnd(outEnd, outStart, outCount);
            GetOvlpStart(ovlpStart, inStart, outStart);
            GetOvlpEnd(ovlpEnd, inEnd, outEnd);
            GetOvlpCount(ovlpCount, ovlpStart, ovlpEnd);
            if (!HasOvlp(ovlpStart, ovlpEnd))
            {
                return 1; // no overlap found
            }

            GetIoStrides(inStride, inCount, sizeof(T));
            GetIoStrides(outStride, outCount, sizeof(T));

            GetRltvOvlpStartPos(inRltvOvlpStartPos, inMemStartNC, ovlpStart);
            GetRltvOvlpStartPos(outRltvOvlpStartPos, outMemStartNC, ovlpStart);
        }
        // row-major ==> col-major mode
        else if (inIsRowMajor && !outIsRowMajor)
        {
            Dims revOutStart(outStart);
            Dims revOutCount(outCount);

            std::reverse(outMemStartNC.begin(), outMemStartNC.end());
            std::reverse(outMemCountNC.begin(), outMemCountNC.end());

            GetInEnd(inEnd, inStart, inCount);
            GetOutEnd(outEnd, revOutStart, revOutCount);
            GetOvlpStart(ovlpStart, inStart, revOutStart);
            GetOvlpEnd(ovlpEnd, inEnd, outEnd);
            GetOvlpCount(ovlpCount, ovlpStart, ovlpEnd);
            if (!HasOvlp(ovlpStart, ovlpEnd))
            {
                return 1; // no overlap found
            }

            // get normal order inStride
            GetIoStrides(inStride, inMemCountNC, sizeof(T));

            // calulate reversed order outStride
            GetIoStrides(outStride, outMemCountNC, sizeof(T));
            // reverse outStride so that outStride aligns to inStride
            std::reverse(outStride.begin(), outStride.end());

            // get normal order inOvlpStart
            GetRltvOvlpStartPos(inRltvOvlpStartPos, inMemStartNC, ovlpStart);

            // get reversed order outOvlpStart
            Dims revOvlpStart(ovlpStart);
            std::reverse(revOvlpStart.begin(), revOvlpStart.end());
            GetRltvOvlpStartPos(outRltvOvlpStartPos, outMemStartNC,
                                revOvlpStart);
        }
        // col-major ==> row-major mode
        else if (!inIsRowMajor && outIsRowMajor)
        {
            Dims revInStart(inStart);
            Dims revInCount(inCount);
            std::reverse(inMemStartNC.begin(), inMemStartNC.end());
            std::reverse(inMemCountNC.begin(), inMemCountNC.end());

            GetInEnd(inEnd, revInStart, revInCount);
            GetOutEnd(outEnd, outStart, outCount);
            GetOvlpStart(ovlpStart, revInStart, outStart);
            GetOvlpEnd(ovlpEnd, inEnd, outEnd);
            GetOvlpCount(ovlpCount, ovlpStart, ovlpEnd);
            if (!HasOvlp(ovlpStart, ovlpEnd))
            {
                return 1; // no overlap found
            }

            // get normal order outStride
            GetIoStrides(outStride, outMemCountNC, sizeof(T));

            // calculate reversed inStride
            GetIoStrides(inStride, inMemCountNC, sizeof(T));
            // reverse inStride so that inStride aligns to outStride
            std::reverse(inStride.begin(), inStride.end());

            // get reversed order inOvlpStart
            Dims revOvlpStart(ovlpStart);
            std::reverse(revOvlpStart.begin(), revOvlpStart.end());
            GetRltvOvlpStartPos(inRltvOvlpStartPos, inMemStartNC, revOvlpStart);
            // get normal order outOvlpStart
            GetRltvOvlpStartPos(outRltvOvlpStartPos, outMemStartNC, ovlpStart);
        }

        inOvlpBase = in;
        outOvlpBase = out;
        // Same Endian"
        if (inIsLittleEndian == outIsLittleEndian)
        {
            if (!safeMode)
            {
                NdCopyRecurDFNonSeqDynamic(0, inOvlpBase, outOvlpBase,
                                           inRltvOvlpStartPos,
                                           outRltvOvlpStartPos, inStride,
                                           outStride, ovlpCount, sizeof(T));
            }
            else
            {
                NdCopyIterDFDynamic(inOvlpBase, outOvlpBase, inRltvOvlpStartPos,
                                    outRltvOvlpStartPos, inStride, outStride,
                                    ovlpCount, sizeof(T));
            }
        }
        // different Endian"
        else
        {
            if (!safeMode)
            {
                NdCopyRecurDFNonSeqDynamicRevEndian(
                    0, inOvlpBase, outOvlpBase, inRltvOvlpStartPos,
                    outRltvOvlpStartPos, inStride, outStride, ovlpCount,
                    sizeof(T));
            }
            else
            {
                NdCopyIterDFDynamicRevEndian(inOvlpBase, outOvlpBase,
                                             inRltvOvlpStartPos,
                                             outRltvOvlpStartPos, inStride,
                                             outStride, ovlpCount, sizeof(T));
            }
        }
    }
    return 0;
}
//*************** End of NdCopy() and its 8 helpers ***************

template <class T>
size_t PayloadSize(const T * /*data*/, const Dims &count) noexcept
{
    const bool isZeros = std::all_of(count.begin(), count.end(),
                                     [](const size_t i) { return i == 0; });

    if (isZeros)
    {
        return sizeof(T);
    }

    return GetTotalSize(count) * sizeof(T);
}

template <>
inline size_t PayloadSize<std::string>(const std::string *data,
                                       const Dims & /*count*/) noexcept
{
    return data->size() + 2; // 2 bytes for the string size
}

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMEMORY_INL_ */
