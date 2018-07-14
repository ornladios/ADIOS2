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
#include <algorithm> //std::copy
#include <cstring>   //std::memcpy
#include <thread>
/// \endcond

namespace adios2
{
namespace helper
{

template <class T>
void InsertToBuffer(std::vector<char> &buffer, const T *source,
                    const size_t elements) noexcept
{
    const char *src = reinterpret_cast<const char *>(source);
    buffer.insert(buffer.end(), src, src + elements * sizeof(T));
}

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
T ReadValue(const std::vector<char> &buffer, size_t &position) noexcept
{
    T value;
    CopyFromBuffer(buffer, position, &value);
    return value;
}

template <class T>
void ClipContiguousMemory(T *dest, const Dims &destStart, const Dims &destCount,
                          const std::vector<char> &contiguousMemory,
                          const Box<Dims> &blockBox,
                          const Box<Dims> &intersectionBox,
                          const bool isRowMajor, const bool reverseDimensions)
{
    auto lf_ClipRowMajor = [](
        T *dest, const Dims &destStart, const Dims &destCount,
        const std::vector<char> &contiguousMemory, const Box<Dims> &blockBox,
        const Box<Dims> &intersectionBox, const bool isRowMajor,
        const bool reverseDimensions) {

        const Dims &start = intersectionBox.first;
        const Dims &end = intersectionBox.second;
        const size_t stride = (end.back() - start.back() + 1) * sizeof(T);

        Dims currentPoint(start); // current point for memory copy
        const Box<Dims> selectionBox =
            helper::StartEndBox(destStart, destCount, reverseDimensions);

        const size_t dimensions = start.size();
        bool run = true;

        const size_t intersectionStart =
            helper::LinearIndex(blockBox, intersectionBox.first, true) *
            sizeof(T);

        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t contiguousStart =
                helper::LinearIndex(blockBox, currentPoint, true) * sizeof(T) -
                intersectionStart;

            const size_t variableStart =
                helper::LinearIndex(selectionBox, currentPoint, true) *
                sizeof(T);

            char *rawVariableData = reinterpret_cast<char *>(dest);

            std::copy(contiguousMemory.begin() + contiguousStart,
                      contiguousMemory.begin() + contiguousStart + stride,
                      rawVariableData + variableStart);

            // here update each index recursively, always starting from the 2nd
            // fastest changing index, since fastest changing index is the
            // continuous part in the previous std::copy
            size_t p = dimensions - 2;
            while (true)
            {
                ++currentPoint[p];
                if (currentPoint[p] > end[p])
                {
                    if (p == 0)
                    {
                        run = false; // we are done
                        break;
                    }
                    else
                    {
                        currentPoint[p] = start[p];
                        --p;
                    }
                }
                else
                {
                    break; // break inner p loop
                }
            } // dimension index update
        }     // run

    };

    auto lf_ClipColumnMajor =
        [](T *dest, const Dims &destStart, const Dims &destCount,
           const std::vector<char> &contiguousMemory, const Box<Dims> &blockBox,
           const Box<Dims> &intersectionBox, const bool isRowMajor,
           const bool reverseDimensions)

    {
        const Dims &start = intersectionBox.first;
        const Dims &end = intersectionBox.second;
        const size_t stride = (end.front() - start.front() + 1) * sizeof(T);

        Dims currentPoint(start); // current point for memory copy

        const Box<Dims> selectionBox =
            helper::StartEndBox(destStart, destCount, reverseDimensions);

        const size_t dimensions = start.size();
        bool run = true;

        const size_t intersectionStart =
            helper::LinearIndex(blockBox, intersectionBox.first, false) *
            sizeof(T);

        while (run)
        {
            // here copy current linear memory between currentPoint and end
            const size_t contiguousStart =
                helper::LinearIndex(blockBox, currentPoint, false) * sizeof(T) -
                intersectionStart;

            const size_t variableStart =
                helper::LinearIndex(selectionBox, currentPoint, false) *
                sizeof(T);

            char *rawVariableData = reinterpret_cast<char *>(dest);

            std::copy(contiguousMemory.begin() + contiguousStart,
                      contiguousMemory.begin() + contiguousStart + stride,
                      rawVariableData + variableStart);

            // here update each index recursively, always starting from the 2nd
            // fastest changing index, since fastest changing index is the
            // continuous part in the previous std::copy
            size_t p = 1;
            while (true)
            {
                ++currentPoint[p];
                if (currentPoint[p] > end[p])
                {
                    if (p == dimensions - 1)
                    {
                        run = false; // we are done
                        break;
                    }
                    currentPoint[p] = start[p];
                    ++p;
                }
                else
                {
                    break; // break inner p loop
                }
            } // dimension index update
        }

    };

    const Dims &start = intersectionBox.first;
    if (start.size() == 1) // 1D copy memory
    {
        // normalize intersection start with variable.m_Start
        const size_t normalizedStart =
            (start.front() - destStart.front()) * sizeof(T);
        char *rawVariableData = reinterpret_cast<char *>(dest);

        std::copy(contiguousMemory.begin(), contiguousMemory.end(),
                  rawVariableData + normalizedStart);
        return;
    }

    if (isRowMajor) // stored with C, C++, Python
    {
        lf_ClipRowMajor(dest, destStart, destCount, contiguousMemory, blockBox,
                        intersectionBox, isRowMajor, reverseDimensions);
    }
    else // stored with Fortran, R
    {
        lf_ClipColumnMajor(dest, destStart, destCount, contiguousMemory,
                           blockBox, intersectionBox, isRowMajor,
                           reverseDimensions);
    }
}

template <class T>
void Resize(std::vector<T> &vec, const size_t dataSize, const bool debugMode,
            const std::string hint, T value)
{
    if (debugMode)
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
    else
    {
        vec.reserve(dataSize);
        vec.resize(dataSize, value);
    }
}
//***************Start of NdCopy() and its 8 helpers ***************
// Author:Shawn Yang, shawnyang610@gmail.com
// NdCopyRecurDFSeqPadding(): helper function
// Copys n-dimensional Data from input to output in the same major and
// endianess.
// It looks for the largest contiguous data block size in the overlap (by its
// helper
// functions) and copies to the output buffer in blocks. the memory address
// calculation complexity for copying each block is minimized to O(1), which is
// independent of the number of dimensions.
void NdCopyRecurDFSeqPadding(size_t curDim, char *&inOvlpBase,
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
// Copys n-dimensional Data from input to output in the same major but in
// reversed
// endianess. the memory address calculation complexity for copying each element
// is
// minimized to average O(1), which is independent of the number of dimensions.

void NdCopyRecurDFSeqPaddingRevEndian(size_t curDim, char *&inOvlpBase,
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
            NdCopyRecurDFSeqPaddingRevEndian(
                curDim + 1, inOvlpBase, outOvlpBase, inOvlpGapSize,
                outOvlpGapSize, ovlpCount, minCountDim, blockSize, elmSize,
                numElmsPerBlock);
    }
    inOvlpBase += inOvlpGapSize[curDim];
    outOvlpBase += outOvlpGapSize[curDim];
}

// NdCopyRecurDFNonSeqDynamic(): helper function
// Copys n-dimensional Data from input to output in the same Endianess but in
// the
// reversed major. the memory address calculation complexity for copying each
// element is
// minimized to average O(1), which is independent of the number of dimensions.
void NdCopyRecurDFNonSeqDynamic(size_t curDim, char *inBase, char *outBase,
                                Dims &inRltvOvlpSPos, Dims &outRltvOvlpSPos,
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
            NdCopyRecurDFNonSeqDynamic(
                curDim + 1,
                inBase + (inRltvOvlpSPos[curDim] + i) * inStride[curDim],
                outBase + (outRltvOvlpSPos[curDim] + i) * outStride[curDim],
                inRltvOvlpSPos, outRltvOvlpSPos, inStride, outStride, ovlpCount,
                elmSize);
    }
}

// NdCopyRecurDFNonSeqDynamicRevEndian(): helper function
// Copys n-dimensional Data from input to output in the reversed Endianess and
// Major.
// The memory address calculation complexity for copying each element is
// minimized to average O(1), which is independent of the number of dimensions.

void NdCopyRecurDFNonSeqDynamicRevEndian(size_t curDim, char *inBase,
                                         char *outBase, Dims &inRltvOvlpSPos,
                                         Dims &outRltvOvlpSPos, Dims &inStride,
                                         Dims &outStride, Dims &ovlpCount,
                                         size_t elmSize)
{
    if (curDim == inStride.size())
    {
        // the following for-loop block is the only difference from the original
        // flippedCopyCat
        for (size_t i = 0; i < elmSize; i++)
        {
            // memcpy(outBase+i, inBase+elmSize-1-i, 1);
            outBase[i] = inBase[elmSize - 1 - i];
        }
    }
    else
    {
        for (size_t i = 0; i < ovlpCount[curDim]; i++)
            NdCopyRecurDFNonSeqDynamicRevEndian(
                curDim + 1,
                inBase + (inRltvOvlpSPos[curDim] + i) * inStride[curDim],
                outBase + (outRltvOvlpSPos[curDim] + i) * outStride[curDim],
                inRltvOvlpSPos, outRltvOvlpSPos, inStride, outStride, ovlpCount,
                elmSize);
    }
}

void NdCopyIterDFSeqPadding(char *&inOvlpBase, char *&outOvlpBase,
                            Dims &inOvlpGapSize, Dims &outOvlpGapSize,
                            Dims &ovlpCount, size_t minContDim,
                            size_t blockSize)
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
                return;
            inOvlpBase += inOvlpGapSize[curDim];
            outOvlpBase += outOvlpGapSize[curDim];
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}

static void NdCopyIterDFSeqPaddingRevEndian(
    char *&inOvlpBase, char *&outOvlpBase, Dims &inOvlpGapSize,
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
            if (curDim == 0) // more logical but expensive place for the check
                return;
            inOvlpBase += inOvlpGapSize[curDim];
            outOvlpBase += outOvlpGapSize[curDim];
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}
static void NdCopyIterDFDynamic(char *inBase, char *outBase,
                                Dims &inRltvOvlpSPos, Dims &outRltvOvlpSPos,
                                Dims &inStride, Dims &outStride,
                                Dims &ovlpCount, size_t elmSize)
{
    size_t curDim = 0;
    Dims pos(ovlpCount.size() + 1, 0);
    std::vector<char *> inAddr(ovlpCount.size() + 1);
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
                return;
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}

static void NdCopyIterDFDynamicRevEndian(char *inBase, char *outBase,
                                         Dims &inRltvOvlpSPos,
                                         Dims &outRltvOvlpSPos, Dims &inStride,
                                         Dims &outStride, Dims &ovlpCount,
                                         size_t elmSize)
{
    size_t curDim = 0;
    Dims pos(ovlpCount.size() + 1, 0);
    std::vector<char *> inAddr(ovlpCount.size() + 1);
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
            // memcpy(outBase+i, inBase+elmSize-1-i, 1);
            outAddr[curDim][i] = inAddr[curDim][elmSize - 1 - i];
        }
        do
        {
            if (curDim == 0)
                return;
            pos[curDim] = 0;
            curDim--;
        } while (pos[curDim] == ovlpCount[curDim]);
    }
}

template <class T>
int NdCopy(const Buffer &in, const Dims &inStart, const Dims &inCount,
           bool inIsRowMaj, bool inIsBigEndian, Buffer &out,
           const Dims &outStart, const Dims &outCount, bool outIsRowMaj,
           bool outIsBigEndian, bool safeMode)
{
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
    char *inOvlpBase = nullptr;
    char *outOvlpBase = nullptr;
    auto GetInEnd = [](Dims &inEnd, const Dims &inStart, const Dims &inCount) {
        for (size_t i = 0; i < inStart.size(); i++)
            inEnd[i] = inStart[i] + inCount[i] - 1;
    };
    auto GetOutEnd = [](Dims &outEnd, const Dims &outStart,
                        const Dims &output_count) {
        for (size_t i = 0; i < outStart.size(); i++)
            outEnd[i] = outStart[i] + output_count[i] - 1;
    };
    auto GetOvlpStart = [](Dims &ovlpStart, const Dims &inStart,
                           const Dims &outStart) {
        for (size_t i = 0; i < ovlpStart.size(); i++)
            ovlpStart[i] = inStart[i] > outStart[i] ? inStart[i] : outStart[i];
    };
    auto GetOvlpEnd = [](Dims &ovlpEnd, Dims &inEnd, Dims &outEnd) {
        for (size_t i = 0; i < ovlpEnd.size(); i++)
            ovlpEnd[i] = inEnd[i] < outEnd[i] ? inEnd[i] : outEnd[i];
    };
    auto GetOvlpCount = [](Dims &ovlpCount, Dims &ovlpStart, Dims &ovlpEnd) {
        for (size_t i = 0; i < ovlpCount.size(); i++)
            ovlpCount[i] = ovlpEnd[i] - ovlpStart[i] + 1;
    };
    auto HasOvlp = [](Dims &ovlpStart, Dims &ovlpEnd) {
        for (size_t i = 0; i < ovlpStart.size(); i++)
            if (ovlpEnd[i] < ovlpStart[i])
                return false;
        return true;
    };

    auto GetIoStrides = [](Dims &ioStride, const Dims &ioCount,
                           size_t elmSize) {
        // ioStride[i] holds the total number of elements under each element
        // of the i'th dimension
        ioStride[ioStride.size() - 1] = elmSize;
        if (ioStride.size() > 1)
            ioStride[ioStride.size() - 2] =
                ioCount[ioStride.size() - 1] * elmSize;
        if (ioStride.size() > 2)
        {
            size_t i = ioStride.size() - 3;
            while (true)
            {
                ioStride[i] = ioCount[i + 1] * ioStride[i + 1];
                if (i == 0)
                    break;
                else
                    i--;
            }
        }
    };

    auto GetIoOvlpBase = [](char *&ioOvlpBase, const Buffer &io,
                            const Dims &ioStart, Dims &ioStride,
                            Dims &ovlpStart) {
        ioOvlpBase = (char *)io.data();
        for (size_t i = 0; i < ioStart.size(); i++)
            ioOvlpBase = ioOvlpBase + (ovlpStart[i] - ioStart[i]) * ioStride[i];
    };
    auto GetIoOvlpGapSize = [](Dims &ioOvlpGapSize, Dims &ioStride,
                               const Dims &ioCount, Dims &ovlpCount) {
        for (size_t i = 0; i < ioOvlpGapSize.size(); i++)
            ioOvlpGapSize[i] = (ioCount[i] - ovlpCount[i]) * ioStride[i];
    };
    auto GetMinContDim = [](const Dims &inCount, const Dims outCount,
                            Dims &ovlpCount) {
        //    note: minContDim is the first index where its input box and
        //    overlap box
        //    are not fully match. therefore all data below this branch is
        //    continous
        //    and this determins the Biggest continuous block size - Each
        //    element of the
        //    current dimension.
        size_t i = ovlpCount.size() - 1;
        while (true)
        {
            if (i == 0)
                break;
            if ((inCount[i] != ovlpCount[i]) || (outCount[i] != ovlpCount[i]))
                break;
            i--;
        }
        return i;
    };
    auto GetBlockSize = [](Dims &ovlpCount, size_t minContDim, size_t elmSize) {
        size_t res = elmSize;
        for (size_t i = minContDim; i < ovlpCount.size(); i++)
            res *= ovlpCount[i];
        return res;
    };

    auto GetRltvOvlpStartPos = [](Dims &ioRltvOvlpStart, const Dims &ioStart,
                                  Dims &ovlpStart) {
        for (size_t i = 0; i < ioStart.size(); i++)
            ioRltvOvlpStart[i] = ovlpStart[i] - ioStart[i];
    };

    // main flow
    // row-major ==> row-major mode
    // algrithm optimizations:
    // 1. contigous data copying
    // 2. mem pointer arithmetics by sequential padding. O(1) overhead/block
    if (inIsRowMaj == true && outIsRowMaj == true)
    {
        GetInEnd(inEnd, inStart, inCount);
        GetOutEnd(outEnd, outStart, outCount);
        GetOvlpStart(ovlpStart, inStart, outStart);
        GetOvlpEnd(ovlpEnd, inEnd, outEnd);
        GetOvlpCount(ovlpCount, ovlpStart, ovlpEnd);
        if (!HasOvlp(ovlpStart, ovlpEnd))
            return 1; // no overlap found
        GetIoStrides(inStride, inCount, sizeof(T));
        GetIoStrides(outStride, outCount, sizeof(T));
        GetIoOvlpGapSize(inOvlpGapSize, inStride, inCount, ovlpCount);
        GetIoOvlpGapSize(outOvlpGapSize, outStride, outCount, ovlpCount);
        GetIoOvlpBase(inOvlpBase, in, inStart, inStride, ovlpStart);
        GetIoOvlpBase(outOvlpBase, out, outStart, outStride, ovlpStart);
        minContDim = GetMinContDim(inCount, outCount, ovlpCount);
        blockSize = GetBlockSize(ovlpCount, minContDim, sizeof(T));
        // same endianess mode: most optimized, contiguous data copying
        // algorithm used.
        if (inIsBigEndian == outIsBigEndian)
        {
            // most efficient algm
            // warning: number of function stacks used is number of dimensions
            // of data.
            if (!safeMode)
                NdCopyRecurDFSeqPadding(0, inOvlpBase, outOvlpBase,
                                        inOvlpGapSize, outOvlpGapSize,
                                        ovlpCount, minContDim, blockSize);
            else // safeMode
                //      //alternative iterative version, 10% slower then
                //      recursive
                //      //use it when very high demension is used.
                NdCopyIterDFSeqPadding(inOvlpBase, outOvlpBase, inOvlpGapSize,
                                       outOvlpGapSize, ovlpCount, minContDim,
                                       blockSize);
        }
        // different endianess mode
        else
        {
            if (!safeMode)
                NdCopyRecurDFSeqPaddingRevEndian(
                    0, inOvlpBase, outOvlpBase, inOvlpGapSize, outOvlpGapSize,
                    ovlpCount, minContDim, blockSize, sizeof(T),
                    blockSize / sizeof(T));
            else
                NdCopyIterDFSeqPaddingRevEndian(
                    inOvlpBase, outOvlpBase, inOvlpGapSize, outOvlpGapSize,
                    ovlpCount, minContDim, blockSize, sizeof(T),
                    blockSize / sizeof(T));
        }
    }

    // Copying modes involing col-major
    // algorithm optimization:
    // 1. mem ptr arithmetics: O(1) overhead per block, dynamic/non-sequential
    // padding
    else
    {
        Dims revInCount(inCount);
        Dims revOutCount(outCount);
        GetInEnd(inEnd, inStart, inCount);
        GetOutEnd(outEnd, outStart, outCount);
        GetOvlpStart(ovlpStart, inStart, outStart);
        GetOvlpEnd(ovlpEnd, inEnd, outEnd);
        GetOvlpCount(ovlpCount, ovlpStart, ovlpEnd);
        if (!HasOvlp(ovlpStart, ovlpEnd))
            return 1; // no overlap found
        // col-major ==> col-major mode
        if (!inIsRowMaj && !outIsRowMaj)
        {
            std::reverse(revInCount.begin(), revInCount.end());
            GetIoStrides(inStride, revInCount, sizeof(T));
            std::reverse(inStride.begin(), inStride.end());
            std::reverse(revOutCount.begin(), revOutCount.end());
            GetIoStrides(outStride, revOutCount, sizeof(T));
            std::reverse(outStride.begin(), outStride.end());
        }
        // row-major ==> col-major mode
        else if (inIsRowMaj && !outIsRowMaj)
        {
            GetIoStrides(inStride, inCount, sizeof(T));
            std::reverse(revOutCount.begin(), revOutCount.end());
            GetIoStrides(outStride, revOutCount, sizeof(T));
            std::reverse(outStride.begin(), outStride.end());
        }
        // col-major ==> row-major mode
        else if (!inIsRowMaj && outIsRowMaj)
        {
            std::reverse(revInCount.begin(), revInCount.end());
            GetIoStrides(inStride, revInCount, sizeof(T));
            std::reverse(inStride.begin(), inStride.end());
            GetIoStrides(outStride, outCount, sizeof(T));
        }
        GetRltvOvlpStartPos(inRltvOvlpStartPos, inStart, ovlpStart);
        GetRltvOvlpStartPos(outRltvOvlpStartPos, outStart, ovlpStart);
        inOvlpBase = (char *)in.data();
        outOvlpBase = (char *)out.data();
        // Same Endian"
        if (inIsBigEndian == outIsBigEndian)
        {
            if (!safeMode)
                NdCopyRecurDFNonSeqDynamic(0, inOvlpBase, outOvlpBase,
                                           inRltvOvlpStartPos,
                                           outRltvOvlpStartPos, inStride,
                                           outStride, ovlpCount, sizeof(T));
            else
                NdCopyIterDFDynamic(inOvlpBase, outOvlpBase, inRltvOvlpStartPos,
                                    outRltvOvlpStartPos, inStride, outStride,
                                    ovlpCount, sizeof(T));
        }
        // different Endian"
        else
        {
            if (!safeMode)
                NdCopyRecurDFNonSeqDynamicRevEndian(
                    0, inOvlpBase, outOvlpBase, inRltvOvlpStartPos,
                    outRltvOvlpStartPos, inStride, outStride, ovlpCount,
                    sizeof(T));
            else
                NdCopyIterDFDynamicRevEndian(inOvlpBase, outOvlpBase,
                                             inRltvOvlpStartPos,
                                             outRltvOvlpStartPos, inStride,
                                             outStride, ovlpCount, sizeof(T));
        }
    }
    return 0;
}
//*************** End of NdCopy() and its 8 helpers ***************

} // end namespace helper
} // end namespace adios2

#endif /* ADIOS2_HELPER_ADIOSMEMORY_INL_ */
