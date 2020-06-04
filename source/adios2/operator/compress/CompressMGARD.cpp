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

#include <cstring> //std::memcpy

#include <mgard_api.h>

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressMGARD::CompressMGARD(const Params &parameters)
: Operator("mgard", parameters)
{
}

size_t CompressMGARD::Compress(const void *dataIn, const Dims &dimensions,
                               const size_t elementSize, DataType type,
                               void *bufferOut, const Params &parameters,
                               Params &info) const
{
    const size_t ndims = dimensions.size();

    if (ndims > 3)
    {
        throw std::invalid_argument("ERROR: ADIOS2 MGARD compression: no more "
                                    "than 3-dimensions is supported.\n");
    }

    // set type
    int mgardType = -1;
    if (type == helper::GetDataType<double>())
    {
        mgardType = 1;
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: ADIOS2 operator "
            "MGARD only supports double precision, in call to Put\n");
    }

    int r[3];
    r[0] = 0;
    r[1] = 0;
    r[2] = 0;

    for (size_t i = 0; i < ndims; i++)
    {
        r[ndims - i - 1] = static_cast<int>(dimensions[i]);
    }

    // Parameters
    bool hasTolerance = false;
    double tolerance, s = 0.0;
    auto itAccuracy = parameters.find("accuracy");
    if (itAccuracy != parameters.end())
    {
        tolerance = std::stod(itAccuracy->second);
        hasTolerance = true;
    }
    auto itTolerance = parameters.find("tolerance");
    if (itTolerance != parameters.end())
    {
        tolerance = std::stod(itTolerance->second);
        hasTolerance = true;
    }
    if (!hasTolerance)
    {
        throw std::invalid_argument("ERROR: missing mandatory parameter "
                                    "tolerance for MGARD compression "
                                    "operator\n");
    }
    auto itSParameter = parameters.find("s");
    if (itSParameter != parameters.end())
    {
        s = std::stod(itSParameter->second);
    }

    int sizeOut = 0;
    unsigned char *dataOutPtr = mgard_compress(
        mgardType, const_cast<double *>(static_cast<const double *>(dataIn)),
        sizeOut, r[0], r[1], r[2], tolerance, s);

    const size_t sizeOutT = static_cast<size_t>(sizeOut);
    std::memcpy(bufferOut, dataOutPtr, sizeOutT);

    return sizeOutT;
}

size_t CompressMGARD::Decompress(const void *bufferIn, const size_t sizeIn,
                                 void *dataOut, const Dims &dimensions,
                                 DataType type,
                                 const Params & /*parameters*/) const
{
    int mgardType = -1;
    size_t elementSize = 0;
    double quantizer = 0.0;

    if (type == helper::GetDataType<double>())
    {
        mgardType = 1;
        elementSize = 8;
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: ADIOS2 operator "
            "MGARD only supports double precision, in call to Get\n");
    }

    const size_t ndims = dimensions.size();
    int r[3];
    r[0] = 0;
    r[1] = 0;
    r[2] = 0;

    for (size_t i = 0; i < ndims; i++)
    {
        r[ndims - i - 1] = static_cast<int>(dimensions[i]);
    }

    void *dataPtr = mgard_decompress(
        mgardType, quantizer,
        reinterpret_cast<unsigned char *>(const_cast<void *>(bufferIn)),
        static_cast<int>(sizeIn), r[0], r[1], r[2], 0.0);

    const size_t dataSizeBytes = helper::GetTotalSize(dimensions) * elementSize;
    std::memcpy(dataOut, dataPtr, dataSizeBytes);

    return static_cast<size_t>(dataSizeBytes);
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
