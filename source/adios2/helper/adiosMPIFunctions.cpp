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

#include "adios2/common/ADIOSMPI.h"
#include "adios2/common/ADIOSTypes.h"

#include "adios2/helper/adiosString.h"

namespace adios2
{
namespace helper
{

void CheckMPIReturn(const int value, const std::string &hint)
{
    if (value == MPI_SUCCESS)
    {
        return;
    }

    std::string error;
    switch (value)
    {
    case MPI_ERR_COMM:
        error = "MPI_ERR_COMM";
        break;
    case MPI_ERR_INTERN:
        error = "MPI_ERR_INTERN";
        break;
    default:
        error = "MPI_ERR number: " + std::to_string(value);
    }

    throw std::runtime_error("ERROR: ADIOS2 detected " + error + ", " + hint);
}

} // end namespace helper
} // end namespace adios2
