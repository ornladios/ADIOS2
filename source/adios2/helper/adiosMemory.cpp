/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.cpp
 *
 *  Created on: Oct 11, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosMemory.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace helper
{

void ClipContiguousMemoryRowMajor(char *destination,
                                  const Box<Dims> &destinationBox,
                                  const char *source,
                                  const Box<Dims> &sourceBox,
                                  const Box<Dims> &intersectionBox) noexcept
{
    const Dims &interStart = intersectionBox.first;
    const Dims &interEnd = intersectionBox.second;
    const Dims &sourceStart = sourceBox.first;
    const Dims &sourceEnd = sourceBox.second;
    const size_t dimensions = interStart.size();

    // loop through intersection start and end and check if it's equal to the
    // sourceBox
    size_t stride = interEnd[dimensions - 1] - interStart[dimensions - 1] + 1;
    size_t startCoord;
    for (startCoord = dimensions - 2; startCoord >= 0; --startCoord)
    {
        // same as source
        if (interStart[startCoord] == sourceStart[startCoord] &&
            interEnd[startCoord] == sourceEnd[startCoord])
        {
            stride *= (interEnd[startCoord] - interStart[startCoord] + 1);
        }
        else
        {
            break;
        }
    }

    Dims currentPoint(interStart); // current point for memory copy
    const size_t interOffset = helper::LinearIndex(sourceBox, interStart, true);
    bool run = true;

    while (run)
    {
        // here copy current linear memory between currentPoint and end
        const size_t sourceOffset =
            helper::LinearIndex(sourceBox, currentPoint, true) - interOffset;

        const size_t destinationOffset =
            helper::LinearIndex(destinationBox, currentPoint, true);

        std::copy(source + sourceOffset, source + sourceOffset + stride,
                  destination + destinationOffset);

        size_t p = startCoord;
        while (true)
        {
            ++currentPoint[p];
            if (currentPoint[p] > interEnd[p])
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

void ClipContiguousMemory(char *destination, const Box<Dims> &destinationBox,
                          const char *source, const Box<Dims> &sourceBox,
                          const bool isRowMajor) noexcept
{
    const Box<Dims> intersectionBox =
        helper::IntersectionBox(destinationBox, sourceBox);
    if (intersectionBox.first.empty() && intersectionBox.second.empty())
    {
        return;
    }
}

void CopyMemory(char *dest, const Dims &destStart, const Dims &destCount,
                char *src, const Dims &srcStart, const Dims &srcCount,
                const bool destIsRowMajor, const bool srcIsRowMajor,
                const Dims &destMemoryStart, const Dims &destMemoryCount,
                const Dims &srcMemoryStart, const Dims &srcMemoryCount) noexcept
{
    // 1D case
    if (destStart.size() == 1)
    {
        const size_t srcMemoryOffset =
            srcMemoryStart.empty() ? 0 : srcMemoryStart[0];
        const size_t destMemoryOffset =
            destMemoryStart.empty() ? 0 : destMemoryStart[0];

        // need intersection
        const Box<Dims> intersection = helper::IntersectionStartCount(
            srcStart, srcCount, destStart, destCount);

        const size_t destStart = intersection.first[0] - destMemoryOffset;
        const size_t srcStart = intersection.first[0] - srcMemoryOffset;
        const size_t srcCount = intersection.second[0];

        std::copy(src + srcStart, src + srcStart + srcCount, dest + destStart);
        return;
    }
}

} // end namespace helper
} // end namespace adios2
