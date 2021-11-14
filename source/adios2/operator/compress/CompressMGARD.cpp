/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressMGARD.cpp :
 *
 *  Created on: Aug 3, 2018
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "CompressMGARD.h"
#include "adios2/helper/adiosFunctions.h"
#include <MGARDConfig.h>
#include <cstring>
#include <mgard/compress_cuda.hpp>

namespace adios2
{
namespace core
{
namespace compress
{

CompressMGARD::CompressMGARD(const Params &parameters)
: Operator("mgard", COMPRESS_MGARD, parameters)
{
}

size_t CompressMGARD::Operate(const char *dataIn, const Dims &blockStart,
                              const Dims &blockCount, const DataType type,
                              char *bufferOut)
{
    const uint8_t bufferVersion = 1;
    size_t bufferOutOffset = 0;

    MakeCommonHeader(bufferOut, bufferOutOffset, bufferVersion);

    const size_t ndims = blockCount.size();
    if (ndims > 5)
    {
        throw std::invalid_argument("ERROR: ADIOS2 MGARD compression: MGARG "
                                    "only supports up to 5 dimensions");
    }

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

    // set type
    mgard_cuda::data_type mgardType;
    if (type == helper::GetDataType<float>())
    {
        mgardType = mgard_cuda::data_type::Float;
    }
    else if (type == helper::GetDataType<double>())
    {
        mgardType = mgard_cuda::data_type::Double;
    }
    else
    {
        throw std::invalid_argument("ERROR: ADIOS2 operator MGARD only "
                                    "supports float and double types");
    }
    // set type end

    // set mgard style dim info
    mgard_cuda::DIM mgardDim = ndims;
    std::vector<mgard_cuda::SIZE> mgardCount;
    for (const auto &c : blockCount)
    {
        mgardCount.push_back(c);
    }
    // set mgard style dim info end

    // Parameters
    bool hasTolerance = false;
    double tolerance = 0.0;
    double s = 0.0;
    auto errorBoundType = mgard_cuda::error_bound_type::REL;

    auto itAccuracy = parameters.find("accuracy");
    if (itAccuracy != parameters.end())
    {
        tolerance = std::stod(itAccuracy->second);
        hasTolerance = true;
    }
    auto itTolerance = m_Parameters.find("tolerance");
    if (itTolerance != m_Parameters.end())
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
    auto itSParameter = m_Parameters.find("s");
    if (itSParameter != m_Parameters.end())
    {
        s = std::stod(itSParameter->second);
    }
    auto itMode = parameters.find("mode");
    if (itMode != parameters.end())
    {
        if (itMode->second == "ABS")
        {
            errorBoundType = mgard_cuda::error_bound_type::ABS;
        }
        else if (itMode->second == "REL")
        {
            errorBoundType = mgard_cuda::error_bound_type::REL;
        }
    }

    size_t sizeOut = 0;
    void *compressedData = nullptr;
    mgard_cuda::compress(mgardDim, mgardType, mgardCount, tolerance, s,
                         errorBoundType, dataIn, compressedData, sizeOut);
    std::memcpy(bufferOut + bufferOutOffset, compressedData, sizeOut);
    bufferOutOffset += sizeOut;

    if (compressedData)
    {
        free(compressedData);
    }

    return bufferOutOffset;
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

    const size_t sizeOut =
        helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));

    try
    {
        void *dataOutVoid = nullptr;
        mgard_cuda::decompress(bufferIn + bufferInOffset,
                               sizeIn - bufferInOffset, dataOutVoid);
        std::memcpy(dataOut, dataOutVoid, sizeOut);
        if (dataOutVoid)
        {
            free(dataOutVoid);
        }
    }
    catch (...)
    {
        throw(m_VersionInfo);
    }

    return sizeOut;
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
    if (type == DataType::Double)
    {
        return true;
    }
    return false;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
