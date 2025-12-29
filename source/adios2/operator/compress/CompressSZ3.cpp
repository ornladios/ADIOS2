/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressSZ3.cpp
 */

#include "CompressSZ3.h"
#include "CompressNull.h"
#include "adios2/helper/adiosFunctions.h"
#include <cmath>     //std::ceil
#include <ios>       //std::ios_base::failure
#include <stdexcept> //std::invalid_argument

#include <SZ3/api/sz.hpp>

namespace adios2
{
namespace core
{
namespace compress
{

CompressSZ3::CompressSZ3(const Params &parameters)
: Operator("sz3", COMPRESS_SZ3, "compress", parameters)
{
}

size_t CompressSZ3::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                            const DataType varType, char *bufferOut)
{
    const uint8_t bufferVersion = 3;
    size_t bufferOutOffset = 0;

    MakeCommonHeader(bufferOut, bufferOutOffset, bufferVersion);

    // Use dimensions directly - SZ3 supports up to 4 dimensions
    const Dims &convertedDims = blockCount;
    const size_t ndims = convertedDims.size();

    if (ndims > 4)
    {
        helper::Throw<std::invalid_argument>(
            "Operator", "CompressSZ3", "Operate",
            "SZ3 only supports up to 4 dimensions, requested " + std::to_string(ndims));
    }

    // sz3 V3 metadata
    PutParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : convertedDims)
    {
        PutParameter(bufferOut, bufferOutOffset, d);
    }
    PutParameter(bufferOut, bufferOutOffset, varType);
    // sz3 V3 metadata end

    // Create SZ3 configuration based on dimensions
    SZ3::Config conf;
    conf.setDims(convertedDims.begin(), convertedDims.end());

    // Set default error bound mode and value
    conf.errorBoundMode = SZ3::EB_ABS;
    conf.absErrorBound = 1E-4;

    // Parse parameters
    Params::const_iterator it;
    for (it = m_Parameters.begin(); it != m_Parameters.end(); it++)
    {
        if (it->first == "errorboundmode" || it->first == "mode")
        {
            const std::string mode = it->second;
            if (mode == "ABS" || mode == "EB_ABS")
            {
                conf.errorBoundMode = SZ3::EB_ABS;
            }
            else if (mode == "REL" || mode == "EB_REL")
            {
                conf.errorBoundMode = SZ3::EB_REL;
            }
            else if (mode == "PSNR" || mode == "EB_PSNR")
            {
                conf.errorBoundMode = SZ3::EB_PSNR;
            }
            else if (mode == "L2NORM" || mode == "EB_L2NORM" || mode == "NORM")
            {
                conf.errorBoundMode = SZ3::EB_L2NORM;
            }
            else if (mode == "ABS_AND_REL" || mode == "EB_ABS_AND_REL")
            {
                conf.errorBoundMode = SZ3::EB_ABS_AND_REL;
            }
            else if (mode == "ABS_OR_REL" || mode == "EB_ABS_OR_REL")
            {
                conf.errorBoundMode = SZ3::EB_ABS_OR_REL;
            }
            else
            {
                helper::Throw<std::invalid_argument>(
                    "Operator", "CompressSZ3", "Operate",
                    "Parameter errorBoundMode must be ABS, REL, PSNR, L2NORM, "
                    "ABS_AND_REL or ABS_OR_REL");
            }
        }
        else if (it->first == "abserrbound" || it->first == "abs" || it->first == "absolute" ||
                 it->first == "accuracy")
        {
            conf.absErrorBound = std::stod(it->second);
            if (it->first == "abs" || it->first == "absolute" || it->first == "accuracy")
            {
                conf.errorBoundMode = SZ3::EB_ABS;
            }
        }
        else if (it->first == "relerrbound" || it->first == "rel" || it->first == "relative")
        {
            conf.relErrorBound = std::stod(it->second);
            if (it->first == "rel" || it->first == "relative")
            {
                conf.errorBoundMode = SZ3::EB_REL;
            }
        }
        else if (it->first == "psnr" || it->first == "psnrerrbound")
        {
            conf.psnrErrorBound = std::stod(it->second);
            if (it->first == "psnr")
            {
                conf.errorBoundMode = SZ3::EB_PSNR;
            }
        }
        else if (it->first == "l2norm" || it->first == "l2normerrbound" || it->first == "norm")
        {
            conf.l2normErrorBound = std::stod(it->second);
            if (it->first == "l2norm" || it->first == "norm")
            {
                conf.errorBoundMode = SZ3::EB_L2NORM;
            }
        }
        else
        {
            // TODO: ignoring unknown option due to Fortran bindings passing
            // empty parameter
        }
    }

    size_t szBufferSize;
    char *szBuffer = nullptr;

    try
    {
        // Compress based on data type
        if (varType == helper::GetDataType<float>())
        {
            szBuffer = SZ_compress<float>(conf, reinterpret_cast<const float *>(dataIn),
                                          szBufferSize);
        }
        else if (varType == helper::GetDataType<double>())
        {
            szBuffer = SZ_compress<double>(conf, reinterpret_cast<const double *>(dataIn),
                                           szBufferSize);
        }
        else if (varType == helper::GetDataType<std::complex<float>>())
        {
            // Compress complex as two separate arrays (real and imaginary)
            const size_t numElements = helper::GetTotalSize(convertedDims);
            const auto *complexData =
                reinterpret_cast<const std::complex<float> *>(dataIn);
            std::vector<float> realPart(numElements);
            for (size_t i = 0; i < numElements; ++i)
            {
                realPart[i] = complexData[i].real();
            }
            szBuffer = SZ_compress<float>(conf, realPart.data(), szBufferSize);
        }
        else if (varType == helper::GetDataType<std::complex<double>>())
        {
            // Compress complex as two separate arrays (real and imaginary)
            const size_t numElements = helper::GetTotalSize(convertedDims);
            const auto *complexData =
                reinterpret_cast<const std::complex<double> *>(dataIn);
            std::vector<double> realPart(numElements);
            for (size_t i = 0; i < numElements; ++i)
            {
                realPart[i] = complexData[i].real();
            }
            szBuffer = SZ_compress<double>(conf, realPart.data(), szBufferSize);
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Operator", "CompressSZ3", "Operate",
                "SZ3 compressor only supports float or double types");
        }
    }
    catch (const std::exception &e)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressSZ3", "Operate",
                                          std::string("SZ3 compression failed: ") + e.what());
    }

    if (bufferOutOffset + szBufferSize >
        helper::GetTotalSize(blockCount, helper::GetDataTypeSize(varType)))
    {
        // Compressed data is larger than original, use null compressor
        CompressNull c({});
        bufferOutOffset = c.Operate(dataIn, blockStart, blockCount, varType, bufferOut);
    }
    else
    {
        std::memcpy(bufferOut + bufferOutOffset, szBuffer, szBufferSize);
        bufferOutOffset += szBufferSize;
    }

    if (szBuffer)
    {
        delete[] szBuffer;
    }

    return bufferOutOffset;
}

size_t CompressSZ3::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion = GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes

    if (bufferVersion == 3)
    {
        return DecompressV3(bufferIn + bufferInOffset, sizeIn - bufferInOffset, dataOut);
    }
    else
    {
        helper::Throw<std::runtime_error>("Operator", "CompressSZ3", "InverseOperate",
                                          "invalid sz3 buffer version (expected 3, got " +
                                              std::to_string(bufferVersion) + ")");
    }

    return 0;
}

bool CompressSZ3::IsDataTypeValid(const DataType type) const
{
    if (type == DataType::Float || type == DataType::Double ||
        type == DataType::FloatComplex || type == DataType::DoubleComplex)
    {
        return true;
    }
    return false;
}

size_t CompressSZ3::DecompressV3(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    // Current decompression format for SZ3 (version 3)
    size_t bufferInOffset = 0;

    const size_t ndims = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    Dims blockCount(ndims);
    for (size_t i = 0; i < ndims; ++i)
    {
        blockCount[i] = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    }
    const DataType type = GetParameter<DataType>(bufferIn, bufferInOffset);

    // Create SZ3 configuration
    SZ3::Config conf;
    conf.setDims(blockCount.begin(), blockCount.end());

    size_t dataTypeSize = 0;
    void *result = nullptr;

    try
    {
        if (type == helper::GetDataType<float>())
        {
            dataTypeSize = sizeof(float);
            result = SZ_decompress<float>(conf, bufferIn + bufferInOffset,
                                          sizeIn - bufferInOffset);
        }
        else if (type == helper::GetDataType<double>())
        {
            dataTypeSize = sizeof(double);
            result = SZ_decompress<double>(conf, bufferIn + bufferInOffset,
                                           sizeIn - bufferInOffset);
        }
        else if (type == helper::GetDataType<std::complex<float>>())
        {
            dataTypeSize = sizeof(std::complex<float>);
            float *realData = SZ_decompress<float>(conf, bufferIn + bufferInOffset,
                                                   sizeIn - bufferInOffset);
            const size_t numElements = helper::GetTotalSize(blockCount);
            auto *complexOut = reinterpret_cast<std::complex<float> *>(dataOut);
            for (size_t i = 0; i < numElements; ++i)
            {
                complexOut[i] = std::complex<float>(realData[i], 0.0f);
            }
            delete[] realData;
            return numElements * dataTypeSize;
        }
        else if (type == helper::GetDataType<std::complex<double>>())
        {
            dataTypeSize = sizeof(std::complex<double>);
            double *realData = SZ_decompress<double>(conf, bufferIn + bufferInOffset,
                                                     sizeIn - bufferInOffset);
            const size_t numElements = helper::GetTotalSize(blockCount);
            auto *complexOut = reinterpret_cast<std::complex<double> *>(dataOut);
            for (size_t i = 0; i < numElements; ++i)
            {
                complexOut[i] = std::complex<double>(realData[i], 0.0);
            }
            delete[] realData;
            return numElements * dataTypeSize;
        }
        else
        {
            helper::Throw<std::invalid_argument>(
                "Operator", "CompressSZ3", "DecompressV3",
                "SZ3 compressor only supports float or double types");
        }
    }
    catch (const std::exception &e)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressSZ3", "DecompressV3",
                                          std::string("SZ3 decompression failed: ") + e.what());
    }

    if (result == nullptr)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressSZ3", "DecompressV3",
                                          "SZ3 decompression returned null");
    }

    const size_t dataSizeBytes = helper::GetTotalSize(blockCount, dataTypeSize);
    std::memcpy(dataOut, result, dataSizeBytes);
    delete[] reinterpret_cast<char *>(result);

    return dataSizeBytes;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
