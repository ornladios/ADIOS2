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
#include "adios2/helper/adiosFunctions.h"
#include <MGARDConfig.h>
#include <cstring>
#include <mgard_api.h>

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

size_t CompressMGARD::Operate(const char *dataIn, const Dims &blockStart,
                              const Dims &blockCount, const DataType type,
                              char *bufferOut, const Params &parameters)
{
    const uint8_t bufferVersion = 1;
    size_t bufferOutOffset = 0;

    // Universal operator metadata
    PutParameter(bufferOut, bufferOutOffset, OperatorType::MGARD);
    PutParameter(bufferOut, bufferOutOffset, bufferVersion);
    PutParameter(bufferOut, bufferOutOffset, static_cast<uint16_t>(0));
    // Universal operator metadata end

    const size_t ndims = blockCount.size();

    // mgard V1 metadata
    PutParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : blockCount)
    {
        PutParameter(bufferOut, bufferOutOffset, d);
    }
    PutParameter(bufferOut, bufferOutOffset, type);
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(MGARD_VERSION_MAJOR));
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(MGARD_VERSION_MINOR));
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(MGARD_VERSION_PATCH));
    // mgard V1 metadata end

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
    r[0] = 1;
    r[1] = 1;
    r[2] = 1;

    for (size_t i = 0; i < ndims; i++)
    {
        r[ndims - i - 1] = static_cast<int>(blockCount[i]);
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
    unsigned char *dataOutPtr =
        mgard_compress(reinterpret_cast<double *>(const_cast<char *>(dataIn)),
                       sizeOut, r[0], r[1], r[2], tolerance, s);

    std::memcpy(bufferOut + bufferOutOffset, dataOutPtr, sizeOut);
    free(dataOutPtr);
    dataOutPtr = nullptr;

    bufferOutOffset += sizeOut;

    return bufferOutOffset;
}

size_t CompressMGARD::InverseOperate(const char *bufferIn, const size_t sizeIn,
                                     char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion =
        GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes

    if (bufferVersion == 1)
    {
        return DecompressV1(bufferIn + bufferInOffset, sizeIn - bufferInOffset,
                            dataOut);
    }
    else if (bufferVersion == 2)
    {
        // TODO: if a Version 2 mgard buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        throw("unknown mgard buffer version");
    }

    return 0;
}

bool CompressMGARD::IsDataTypeValid(const DataType type) const
{
#define declare_type(T)                                                        \
    if (helper::GetDataType<T>() == type)                                      \
    {                                                                          \
        return true;                                                           \
    }
    ADIOS2_FOREACH_MGARD_TYPE_1ARG(declare_type)
#undef declare_type
    return false;
}

size_t CompressMGARD::DecompressV1(const char *bufferIn, const size_t sizeIn,
                                   char *dataOut)
{
    // Do NOT remove even if the buffer version is updated. Data might be still
    // in lagacy formats. This function must be kept for backward compatibility.
    // If a newer buffer format is implemented, create another function, e.g.
    // DecompressV2 and keep this function for decompressing lagacy data.

    size_t bufferInOffset = 0;

    const size_t ndims = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    Dims blockCount(ndims);
    for (size_t i = 0; i < ndims; ++i)
    {
        blockCount[i] = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    }
    const DataType type = GetParameter<DataType>(bufferIn, bufferInOffset);
    m_VersionInfo =
        " Data is compressed using MGARD Version " +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) +
        ". Please make sure a compatible version is used for decompression.";

    int mgardType = -1;

    if (type == helper::GetDataType<double>())
    {
        mgardType = 1;
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: ADIOS2 operator "
            "MGARD only supports double precision, in call to Get\n");
    }

    int r[3];
    r[0] = 1;
    r[1] = 1;
    r[2] = 1;

    for (size_t i = 0; i < ndims; i++)
    {
        r[ndims - i - 1] = static_cast<int>(blockCount[i]);
    }

    void *dataPtr = mgard_decompress(
        reinterpret_cast<unsigned char *>(
            const_cast<char *>(bufferIn + bufferInOffset)),
        static_cast<int>(sizeIn - bufferInOffset), r[0], r[1], r[2], 0.0);

    const size_t sizeOut =
        helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));

    std::memcpy(dataOut, dataPtr, sizeOut);

    free(dataPtr);
    dataPtr = nullptr;

    return sizeOut;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
