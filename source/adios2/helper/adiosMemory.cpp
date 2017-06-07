/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMemory.cpp
 *
 *  Created on: May 17, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "adiosMemory.h"

namespace adios
{

int GrowBuffer(const size_t incomingDataSize, const float growthFactor,
               std::vector<char> &buffer, const size_t position)
{
    const size_t currentCapacity = buffer.capacity();
    const size_t availableSpace = currentCapacity - position;
    const double gf = static_cast<double>(growthFactor);

    if (incomingDataSize < availableSpace)
    {
        return 0;
    }

    const size_t neededCapacity = incomingDataSize + position;
    const double numerator = std::log(static_cast<double>(neededCapacity) /
                                      static_cast<double>(currentCapacity));
    const double denominator = std::log(gf);

    const double n = std::ceil(numerator / denominator);
    const size_t newSize =
        static_cast<size_t>(std::ceil(std::pow(gf, n) * currentCapacity));

    try
    {
        buffer.resize(newSize);
    }
    catch (std::bad_alloc &e)
    {
        return -1;
    }

    return 1;
}

} // end namespace adios
