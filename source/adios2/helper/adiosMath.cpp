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

#include <algorithm> //std::transform
#include <cmath>
#include <functional> //std::minus<T>
#include <numeric>    //std::accumulate
#include <utility>    //std::pair

#include "adios2/helper/adiosString.h" //DimsToString

namespace adios2
{

size_t GetTotalSize(const Dims &dimensions) noexcept
{
    return std::accumulate(dimensions.begin(), dimensions.end(), 1,
                           std::multiplies<size_t>());
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

Box<Dims> StartEndBox(const Dims &start, const Dims &count) noexcept
{
    Box<Dims> box;
    box.first = start;
    box.second.reserve(start.size());

    std::transform(start.begin(), start.end(), count.begin(),
                   std::back_inserter(box.second), std::plus<size_t>());

    return box;
}

Box<Dims> StartCountBox(const Dims &start, const Dims &end) noexcept
{
    Box<Dims> box;
    box.first = start;
    box.second.reserve(start.size());

    std::transform(end.begin(), end.end(), start.begin(),
                   std::back_inserter(box.second), std::minus<size_t>());

    return box;
}

Box<Dims> IntersectionBox(const Box<Dims> &box1, const Box<Dims> &box2) noexcept
{
    Box<Dims> intersectionBox;
    const size_t dimensionsSize = box1.first.size();

    for (size_t d = 0; d < dimensionsSize; ++d)
    {
        // Don't intercept
        if (box2.first[d] >= box1.second[d] || box2.second[d] <= box1.first[d])
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
            intersectionBox.second.push_back(box2.second[d] - 1);
        }
        else
        {
            intersectionBox.second.push_back(box1.second[d] - 1);
        }
    }

    return intersectionBox;
}

size_t LinearIndex(const Box<Dims> &localBox, const Dims &point,
                   const bool isRowMajor, const bool isZeroIndex) noexcept
{
    auto lf_RowZero = [](const Dims &count,
                         const Dims &normalizedPoint) -> size_t {

        const size_t countSize = count.size();
        size_t linearIndex = 0;
        size_t product = std::accumulate(count.begin() + 1, count.end(), 1,
                                         std::multiplies<size_t>());

        for (size_t p = 0; p < countSize - 1; ++p)
        {
            linearIndex += normalizedPoint[p] * product;
            product /= count[p + 1];
        }
        linearIndex += normalizedPoint[countSize - 1]; // fastest
        return linearIndex;
    };

    auto lf_ColumnOne = [](const Dims &count,
                           const Dims &normalizedPoint) -> size_t {

        const size_t countSize = count.size();
        size_t linearIndex = 0;
        size_t product = std::accumulate(count.begin(), count.end() - 1, 1,
                                         std::multiplies<size_t>());

        for (size_t p = 1; p < countSize; ++p)
        {
            linearIndex += (normalizedPoint[countSize - p] - 1) * product;
            product /= count[countSize - p];
        }
        linearIndex += (normalizedPoint[0] - 1); // fastest
        return linearIndex;
    };

    const Box<Dims> localBoxStartCount =
        StartCountBox(localBox.first, localBox.second);

    const Dims &start = localBoxStartCount.first;
    const Dims &count = localBoxStartCount.second;

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

    if (isRowMajor && isZeroIndex)
    {
        linearIndex = lf_RowZero(count, normalizedPoint);
    }
    else if (!isRowMajor && !isZeroIndex)
    {
        linearIndex = lf_ColumnOne(count, normalizedPoint);
    }

    return linearIndex;
}

} // end namespace adios2
