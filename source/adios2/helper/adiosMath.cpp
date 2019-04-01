/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMath.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosMath.h"

#include <algorithm> //std::transform, std::reverse
#include <cmath>
#include <functional> //std::minus<T>
#include <iterator>   //std::back_inserter
#include <numeric>    //std::accumulate
#include <utility>    //std::pair

#include "adios2/helper/adiosString.h" //DimsToString

namespace adios2
{
namespace helper
{

size_t GetTotalSize(const Dims &dimensions) noexcept
{
    return std::accumulate(dimensions.begin(), dimensions.end(),
                           static_cast<size_t>(1), std::multiplies<size_t>());
}

bool CheckIndexRange(const int index, const int upperLimit,
                     const int lowerLimit) noexcept
{
    bool inRange = false;
    if (index <= upperLimit && index >= lowerLimit)
    {
        inRange = true;
    }
    return inRange;
}

size_t NextExponentialSize(const size_t requiredSize, const size_t currentSize,
                           const float growthFactor) noexcept
{
    if (currentSize >= requiredSize)
    {
        return currentSize;
    }

    const double growthFactorDouble = static_cast<double>(growthFactor);

    const double numerator = std::log(static_cast<double>(requiredSize) /
                                      static_cast<double>(currentSize));
    const double denominator = std::log(growthFactorDouble);
    const double n = std::ceil(numerator / denominator);

    const size_t nextExponentialSize = static_cast<size_t>(
        std::ceil(std::pow(growthFactorDouble, n) * currentSize));

    return nextExponentialSize;
}

Box<Dims> StartEndBox(const Dims &start, const Dims &count,
                      const bool reverse) noexcept
{
    Box<Dims> box;
    box.first = start;
    const size_t size = start.size();
    box.second.reserve(size);

    for (size_t d = 0; d < size; ++d)
    {
        box.second.push_back(start[d] + count[d] - 1); // end inclusive
    }

    if (reverse)
    {
        std::reverse(box.first.begin(), box.first.end());
        std::reverse(box.second.begin(), box.second.end());
    }

    return box;
}

Box<Dims> StartCountBox(const Dims &start, const Dims &end) noexcept
{
    Box<Dims> box;
    box.first = start;
    const size_t size = start.size();
    box.second.reserve(size);

    for (size_t d = 0; d < size; ++d)
    {
        box.second.push_back(end[d] - start[d] + 1); // end inclusive
    }

    return box;
}

Box<Dims> IntersectionBox(const Box<Dims> &box1, const Box<Dims> &box2) noexcept
{
    Box<Dims> intersectionBox;
    const size_t dimensionsSize = box1.first.size();

    for (size_t d = 0; d < dimensionsSize; ++d)
    {
        // Don't intercept
        if (box2.first[d] > box1.second[d] || box2.second[d] < box1.first[d])
        {
            return intersectionBox;
        }
    }

    // get the intersection box
    intersectionBox.first.reserve(dimensionsSize);
    intersectionBox.second.reserve(dimensionsSize);

    for (size_t d = 0; d < dimensionsSize; ++d)
    {
        // start
        if (box1.first[d] < box2.first[d])
        {
            intersectionBox.first.push_back(box2.first[d]);
        }
        else
        {
            intersectionBox.first.push_back(box1.first[d]);
        }

        // end, must be inclusive
        if (box1.second[d] > box2.second[d])
        {
            intersectionBox.second.push_back(box2.second[d]);
        }
        else
        {
            intersectionBox.second.push_back(box1.second[d]);
        }
    }

    return intersectionBox;
}

Box<Dims> IntersectionStartCount(const Dims &start1, const Dims &count1,
                                 const Dims &start2,
                                 const Dims &count2) noexcept
{
    Box<Dims> intersectionStartCount;
    const size_t dimensionsSize = start1.size();

    for (auto d = 0; d < dimensionsSize; ++d)
    {
        // Don't intercept
        const size_t end1 = start1[d] + count1[d] - 1;
        const size_t end2 = start2[d] + count2[d] - 1;

        if (start2[d] > end1 || end2 < start1[d])
        {
            return intersectionStartCount;
        }
    }

    intersectionStartCount.first.reserve(dimensionsSize);
    intersectionStartCount.second.reserve(dimensionsSize);

    for (auto d = 0; d < dimensionsSize; ++d)
    {
        const size_t intersectionStart =
            (start1[d] < start2[d]) ? start2[d] : start1[d];

        // end, must be inclusive
        const size_t end1 = start1[d] + count1[d] - 1;
        const size_t end2 = start2[d] + count2[d] - 1;
        const size_t intersectionEnd = (end1 > end2) ? end2 : end1;

        intersectionStartCount.first.push_back(intersectionStart);
        intersectionStartCount.second.push_back(intersectionEnd -
                                                intersectionStart + 1);
    }

    return intersectionStartCount;
}

bool IdenticalBoxes(const Box<Dims> &box1, const Box<Dims> &box2) noexcept
{
    const size_t dimensionsSize = box1.first.size();
    for (size_t d = 0; d < dimensionsSize; ++d)
    {
        if (box1.first[d] != box2.first[d] || box1.second[d] != box2.second[d])
        {
            return false;
        }
    }
    return true;
}

bool IsIntersectionContiguousSubarray(const Box<Dims> &blockBox,
                                      const Box<Dims> &intersectionBox,
                                      const bool isRowMajor,
                                      size_t &startOffset) noexcept
{
    const size_t dimensionsSize = blockBox.first.size();
    size_t nElements = 1; // number of elements in dim 1..n-1
    if (dimensionsSize == 0)
    {
        startOffset = 0;
        return true;
    }
    // It is a contiguous subarray iff the dimensions are equal everywhere
    // except in the slowest dimension
    int dStart, dEnd, dSlowest;
    if (isRowMajor)
    {
        // first dimension is slowest
        dSlowest = 0;
        dStart = 1;
        dEnd = static_cast<int>(dimensionsSize - 1);
    }
    else
    {
        // last dimension is slowest
        dStart = 0;
        dEnd = static_cast<int>(dimensionsSize - 2);
        dSlowest = static_cast<int>(dimensionsSize - 1);
    }

    for (size_t d = dStart; d <= dEnd; ++d)
    {
        if (blockBox.first[d] != intersectionBox.first[d] ||
            blockBox.second[d] != intersectionBox.second[d])
        {
            return false;
        }
        nElements *= (blockBox.second[d] - blockBox.first[d] + 1);
    }
    startOffset = (intersectionBox.first[dSlowest] - blockBox.first[dSlowest]) *
                  nElements;
    return true;
}

size_t LinearIndex(const Dims &start, const Dims &count, const Dims &point,
                   const bool isRowMajor) noexcept
{
    auto lf_RowMajor = [](const Dims &count,
                          const Dims &normalizedPoint) -> size_t {
        const size_t countSize = count.size();
        size_t linearIndex = normalizedPoint[countSize - 1]; // fastest
        size_t product = 1;

        for (auto p = countSize - 1; p >= 1; --p)
        {
            product *= count[p];
            linearIndex += normalizedPoint[p - 1] * product;
        }
        return linearIndex;
    };

    auto lf_ColumnMajor = [](const Dims &count,
                             const Dims &normalizedPoint) -> size_t {
        const size_t countSize = count.size();
        size_t linearIndex = normalizedPoint[0]; // fastest
        size_t product = 1;

        for (auto p = 1; p < countSize; ++p)
        {
            product *= count[p - 1];
            linearIndex += normalizedPoint[p] * product;
        }
        return linearIndex;
    };

    if (count.size() == 1)
    {
        return (point[0] - start[0]);
    }

    // normalize the point
    Dims normalizedPoint;
    normalizedPoint.reserve(point.size());
    std::transform(point.begin(), point.end(), start.begin(),
                   std::back_inserter(normalizedPoint), std::minus<size_t>());

    size_t linearIndex = MaxSizeT - 1;

    if (isRowMajor)
    {
        linearIndex = lf_RowMajor(count, normalizedPoint);
    }
    else
    {
        linearIndex = lf_ColumnMajor(count, normalizedPoint);
    }

    return linearIndex;
}

size_t LinearIndex(const Box<Dims> &startEndBox, const Dims &point,
                   const bool isRowMajor) noexcept
{
    const Box<Dims> localBoxStartCount =
        StartCountBox(startEndBox.first, startEndBox.second);

    const Dims &start = localBoxStartCount.first;
    const Dims &count = localBoxStartCount.second;

    return LinearIndex(start, count, point, isRowMajor);
}

size_t GetDistance(const size_t end, const size_t start, const bool debugMode,
                   const std::string &hint)
{
    if (debugMode)
    {
        if (end < start)
        {
            throw std::invalid_argument(
                "ERROR: end position: " + std::to_string(end) +
                " is smaller than start position " + std::to_string(start) +
                ", " + hint);
        }
    }

    return end - start;
}

} // end namespace helper
} // end namespace adios2
