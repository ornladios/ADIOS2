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

#include <cmath>

namespace adios
{

size_t GetTotalSize(const Dims &dimensions) noexcept
{
    size_t product = 1;

    for (const auto dimension : dimensions)
    {
        product *= dimension;
    }

    return product;
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

} // end namespace adios
