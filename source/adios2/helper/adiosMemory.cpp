/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.cpp
 *
 *  Created on: Oct 31, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosMemory.h"

#include <algorithm>
#include <stddef.h> // max_align_t

#include "adios2/helper/adiosType.h"

namespace adios2
{
namespace helper
{

namespace
{

void CopyPayloadStride(const char *src, const size_t payloadStride, char *dest,
                       const bool endianReverse, const std::string destType)
{
#ifdef ADIOS2_HAVE_ENDIAN_REVERSE
    if (endianReverse)
    {
        if (destType == "")
        {
        }
#define declare_type(T)                                                        \
    else if (destType == GetType<T>())                                         \
    {                                                                          \
        CopyEndianReverse<T>(src, payloadStride, reinterpret_cast<T *>(dest)); \
    }

        ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type
    }
    else
    {
        std::copy(src, src + payloadStride, dest);
    }
#else
    std::copy(src, src + payloadStride, dest);
#endif
}

Dims DestDimsFinal(const Dims &destDims, const bool destRowMajor,
                   const bool srcRowMajor)
{
    Dims destDimsFinal = destDims;
    if (srcRowMajor != destRowMajor)
    {
        std::reverse(destDimsFinal.begin(), destDimsFinal.end());
    }
    return destDimsFinal;
}

void ClipRowMajor(char *dest, const Dims &destStart, const Dims &destCount,
                  const bool destRowMajor, const char *src,
                  const Dims &srcStart, const Dims &srcCount,
                  const Dims & /*destMemStart*/, const Dims & /*destMemCount*/,
                  const Dims &srcMemStart, const Dims &srcMemCount,
                  const bool endianReverse, const std::string destType)
{
    const Dims destStartFinal = DestDimsFinal(destStart, destRowMajor, true);
    const Dims destCountFinal = DestDimsFinal(destCount, destRowMajor, true);
    const Box<Dims> intersectionBox = IntersectionStartCount(
        destStartFinal, destCountFinal, srcStart, srcCount);

    const Dims &interStart = intersectionBox.first;
    const Dims &interCount = intersectionBox.second;
    // loop through intersection start and end and check if it's equal to the
    // srcBox contiguous part
    const size_t dimensions = interStart.size();

    size_t stride = interCount.back();
    size_t startCoord = dimensions - 2;
    //    bool isWholeCopy = false;
    //
    //    for (size_t i = dimensions - 1; i >= 0; --i)
    //    {
    //        // same as source
    //        if (interCount[i] == srcCount[i])
    //        {
    //            stride *= interCount[i - 1];
    //            if (startCoord > 0)
    //            {
    //                --startCoord;
    //            }
    //            if (startCoord == 0)
    //            {
    //                isWholeCopy = true;
    //            }
    //        }
    //        else
    //        {
    //            break;
    //        }
    //    }

    /// start iteration
    Dims currentPoint(interStart); // current point for memory copy
    const size_t interOffset =
        LinearIndex(srcStart, srcCount, interStart, true);

    bool run = true;

    while (run)
    {

        // here copy current linear memory between currentPoint and end
        const size_t srcBeginOffset =
            srcMemStart.empty()
                ? LinearIndex(srcStart, srcCount, currentPoint, true) -
                      interOffset
                : LinearIndex(Dims(srcMemCount.size(), 0), srcMemCount,
                              VectorsOp(std::plus<size_t>(),
                                        VectorsOp(std::minus<size_t>(),
                                                  currentPoint, interStart),
                                        srcMemStart),
                              true);

        const size_t destBeginOffset = helper::LinearIndex(
            destStartFinal, destCountFinal, currentPoint, true);

        CopyPayloadStride(src + srcBeginOffset, stride, dest + destBeginOffset,
                          endianReverse, destType);

        size_t p = startCoord;
        while (true)
        {
            ++currentPoint[p];
            if (currentPoint[p] > interStart[p] + interCount[p] - 1)
            {
                if (p == 0)
                {
                    run = false; // we are done
                    break;
                }
                else
                {
                    currentPoint[p] = interStart[p];
                    --p;
                }
            }
            else
            {
                break; // break inner p loop
            }
        } // dimension index update
    }
}

void ClipColumnMajor(char *dest, const Dims &destStart, const Dims &destCount,
                     const bool destRowMajor, const char *src,
                     const Dims &srcStart, const Dims &srcCount,
                     const Dims & /*destMemStart*/,
                     const Dims & /*destMemCount*/, const Dims &srcMemStart,
                     const Dims &srcMemCount, const bool endianReverse,
                     const std::string destType)
{
    const Dims destStartFinal = DestDimsFinal(destStart, destRowMajor, false);
    const Dims destCountFinal = DestDimsFinal(destCount, destRowMajor, false);
    const Box<Dims> intersectionBox = IntersectionStartCount(
        destStartFinal, destCountFinal, srcStart, srcCount);

    const Dims &interStart = intersectionBox.first;
    const Dims &interCount = intersectionBox.second;
    // loop through intersection start and end and check if it's equal to the
    // srcBox contiguous part
    const size_t dimensions = interStart.size();
    size_t stride = interCount.front();
    size_t startCoord = 1;
    //    for (size_t i = 0; i < dimensions; ++i)
    //    {
    //        // same as source
    //        if (interCount[i] == srcCount[i])
    //        {
    //            stride *= interCount[i];
    //            // startCoord = i + 1;
    //        }
    //        else
    //        {
    //            break;
    //        }
    //    }

    /// start iteration
    Dims currentPoint(interStart); // current point for memory copy
    const size_t interOffset =
        LinearIndex(srcStart, srcCount, interStart, false);

    bool run = true;

    while (run)
    {
        // here copy current linear memory between currentPoint and end
        const size_t srcBeginOffset =
            srcMemStart.empty()
                ? LinearIndex(srcStart, srcCount, currentPoint, false) -
                      interOffset
                : LinearIndex(Dims(srcMemCount.size(), 0), srcMemCount,
                              VectorsOp(std::plus<size_t>(),
                                        VectorsOp(std::minus<size_t>(),
                                                  currentPoint, interStart),
                                        srcMemStart),
                              false);

        const size_t destBeginOffset = helper::LinearIndex(
            destStartFinal, destCountFinal, currentPoint, false);

        CopyPayloadStride(src + srcBeginOffset, stride, dest + destBeginOffset,
                          endianReverse, destType);
        size_t p = startCoord;

        while (true)
        {
            ++currentPoint[p];
            if (currentPoint[p] > interStart[p] + interCount[p] - 1)
            {
                if (p == dimensions - 1)
                {
                    run = false; // we are done
                    break;
                }
                else
                {
                    currentPoint[p] = interStart[p];
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

} // end empty namespace

void CopyPayload(char *dest, const Dims &destStart, const Dims &destCount,
                 const bool destRowMajor, const char *src, const Dims &srcStart,
                 const Dims &srcCount, const bool srcRowMajor,
                 const Dims &destMemStart, const Dims &destMemCount,
                 const Dims &srcMemStart, const Dims &srcMemCount,
                 const bool endianReverse, const std::string destType) noexcept
{
    if (srcStart.size() == 1) // 1D copy memory
    {
        const Box<Dims> intersectionBox =
            IntersectionStartCount(destStart, destCount, srcStart, srcCount);
        const Dims &interStart = intersectionBox.first;
        const Dims &interCount = intersectionBox.second;

        const size_t srcBeginOffset =
            srcMemStart.empty()
                ? interStart.front() - srcStart.front()
                : interStart.front() - srcStart.front() + srcMemStart.front();

        const size_t stride = interCount.front();
        const size_t destBeginOffset = interStart.front() - destStart.front();

        CopyPayloadStride(src + srcBeginOffset, stride, dest + destBeginOffset,
                          endianReverse, destType);
        return;
    }

    if (srcRowMajor) // stored with C, C++, Python
    {
        ClipRowMajor(dest, destStart, destCount, destRowMajor, src, srcStart,
                     srcCount, destMemStart, destMemCount, srcMemStart,
                     srcMemCount, endianReverse, destType);
    }
    else // stored with Fortran, R
    {
        ClipColumnMajor(dest, destStart, destCount, destRowMajor, src, srcStart,
                        srcCount, destMemStart, destMemCount, srcMemStart,
                        srcMemCount, endianReverse, destType);
    }
}

size_t PaddingToAlignPointer(const void *ptr)
{
    auto memLocation = reinterpret_cast<std::uintptr_t>(ptr);
    size_t padSize = sizeof(max_align_t) - (memLocation % sizeof(max_align_t));
    if (padSize == sizeof(max_align_t))
    {
        padSize = 0;
    }
    return padSize;
}

} // end namespace helper
} // end namespace adios2
