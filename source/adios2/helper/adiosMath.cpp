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

} // end namespace adios
