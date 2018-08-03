/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressMGARD.cpp :
 *
 *  Created on: Aug 3, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "CompressMGARD.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressMGARD::CompressMGARD(const Params &parameters, const bool debugMode) {}

size_t CompressMGARD::Compress(const void *dataIn, const Dims &dimensions,
                               const size_t elementSize, const std::string type,
                               void *bufferOut, const Params &parameters) const
{
    if (m_DebugMode)
    {
        // check dimensions
    }

    // set type
    int mgardType = -1;
    if (type == "float")
    {
        mgardType = 0;
    }
    else if (type == "double")
    {
        mgardType = 1;
    }
}

size_t CompressMGARD::Decompress(const void *bufferIn, const size_t sizeIn,
                                 void *dataOut, const size_t sizeOut) const
{
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
