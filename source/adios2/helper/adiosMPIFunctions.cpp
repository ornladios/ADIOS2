/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * adiosMPIFunctions.cpp
 *
 *  Created on: Jul 20, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "adiosMPIFunctions.h"
#include "adiosMPIFunctions.tcc"

#include "adios2/ADIOSMPI.h"
#include "adios2/ADIOSTypes.h"

namespace adios2
{

std::vector<int> GetGathervDisplacements(const size_t *counts,
                                         const size_t countsSize)
{
    std::vector<int> displacements(countsSize);
    displacements[0] = 0;

    for (size_t i = 1; i < countsSize; ++i)
    {
        displacements[i] = displacements[i - 1] + counts[i - 1];
    }
    return displacements;
}

} // end namespace adios2
