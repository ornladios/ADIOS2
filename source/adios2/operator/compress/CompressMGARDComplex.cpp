/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressMGARDComplex.cpp :
 *
 *  Created on: Oct 6, 2025
 *      Author: Generated for complex-valued MGARD compression
 */

#include "CompressMGARDComplex.h"
#include "CompressNull.h"
#include "adios2/helper/adiosFunctions.h"
#include <complex>
#include <cstring>
#include <mgard/MGARDConfig.hpp>
#include <mgard/compress_x.hpp>

namespace adios2
{
namespace core
{
namespace compress
{

CompressMGARDComplex::CompressMGARDComplex(const Params &parameters)
: Operator("mgard_complex", COMPRESS_MGARDCOMPLEX, "compress", parameters)
{
}

size_t CompressMGARDComplex::Operate(const char *dataIn, const Dims &blockStart,
                                     const Dims &blockCount, const DataType type, char *bufferOut)
{
    const uint8_t bufferVersion = 1;
    size_t bufferOutOffset = 0;

    MakeCommonHeader(bufferOut, bufferOutOffset, bufferVersion);

    // For complex data, use blockCount directly since we handle real/imag separation manually
    const size_t ndims = blockCount.size();
    if (ndims > 5)
    {
        helper::Throw<std::invalid_argument>("Operator", "CompressMGARDComplex", "Operate",
                                             "MGARD does not support data in " +
                                                 std::to_string(ndims) + " dimensions");
    }

    // mgard V1 metadata
    PutParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : blockCount)
    {
        PutParameter(bufferOut, bufferOutOffset, d);
    }
    PutParameter(bufferOut, bufferOutOffset, type);
    PutParameter(bufferOut, bufferOutOffset, static_cast<uint8_t>(MGARD_VERSION_MAJOR));
    PutParameter(bufferOut, bufferOutOffset, static_cast<uint8_t>(MGARD_VERSION_MINOR));
    PutParameter(bufferOut, bufferOutOffset, static_cast<uint8_t>(MGARD_VERSION_PATCH));
    // mgard V1 metadata end

    // Determine the underlying real type for MGARD
    mgard_x::data_type mgardType;
    size_t elementSize = 0;
    if (type == helper::GetDataType<std::complex<float>>())
    {
        mgardType = mgard_x::data_type::Float;
        elementSize = sizeof(float);
    }
    else if (type == helper::GetDataType<std::complex<double>>())
    {
        mgardType = mgard_x::data_type::Double;
        elementSize = sizeof(double);
    }
    else
    {
        helper::Throw<std::invalid_argument>(
            "Operator", "CompressMGARDComplex", "Operate",
            "MGARD Complex only supports complex<float> and complex<double> types");
    }

    // set mgard style dim info
    mgard_x::DIM mgardDim = ndims;
    std::vector<mgard_x::SIZE> mgardCount;
    for (const auto &c : blockCount)
    {
        mgardCount.push_back(c);
    }

    // Parameters
    bool hasTolerance = false;
    double tolerance = 0.0;
    double s = 0.0;
    auto errorBoundType = mgard_x::error_bound_type::REL;

    // input size under this bound will not compress
    size_t thresholdSize = 100000;

    auto itThreshold = m_Parameters.find("threshold");
    if (itThreshold != m_Parameters.end())
    {
        thresholdSize = std::stod(itThreshold->second);
    }
    auto itAccuracy = m_Parameters.find("accuracy");
    if (itAccuracy != m_Parameters.end())
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
        helper::Throw<std::invalid_argument>("Operator", "CompressMGARDComplex", "Operate",
                                             "missing mandatory parameter tolerance / accuracy");
    }
    auto itSParameter = m_Parameters.find("s");
    if (itSParameter != m_Parameters.end())
    {
        s = std::stod(itSParameter->second);
    }
    auto itMode = m_Parameters.find("mode");
    if (itMode != m_Parameters.end())
    {
        if (itMode->second == "ABS")
        {
            errorBoundType = mgard_x::error_bound_type::ABS;
        }
        else if (itMode->second == "REL")
        {
            errorBoundType = mgard_x::error_bound_type::REL;
        }
    }

    // Calculate sizes
    const size_t totalElements = helper::GetTotalSize(blockCount, 1); // Number of complex elements
    const size_t totalBytes = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));
    const size_t realBytes = totalElements * elementSize; // Size of real or imag component

    if (totalBytes < thresholdSize)
    {
        /* disable compression and add marker in the header*/
        PutParameter(bufferOut, bufferOutOffset, false);
        headerSize = bufferOutOffset;
        return 0;
    }

    mgard_x::Config config;
    config.lossless = mgard_x::lossless_type::Huffman_Zstd;

    // Check for device parameter
    auto itDevice = m_Parameters.find("device");
    if (itDevice != m_Parameters.end())
    {
        if (itDevice->second == "cpu" || itDevice->second == "CPU")
        {
            config.dev_type = mgard_x::device_type::SERIAL;
        }
        // GPU is the default, so we don't need to set it explicitly
    }

    PutParameter(bufferOut, bufferOutOffset, true);

    // Store space for the compressed sizes in header first
    size_t realSizeOffset = bufferOutOffset;
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<size_t>(0)); // Placeholder for real compressed size
    size_t imagSizeOffset = bufferOutOffset;
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<size_t>(0)); // Placeholder for imaginary compressed size

    // Copy complex data into std::vector<char> and separate into real and imaginary parts
    std::vector<char> combinedData(2 * realBytes); // Space for both real and imaginary parts

    if (type == helper::GetDataType<std::complex<float>>())
    {
        const std::complex<float> *complexData =
            reinterpret_cast<const std::complex<float> *>(dataIn);
        float *realPtr = reinterpret_cast<float *>(combinedData.data());
        float *imagPtr = reinterpret_cast<float *>(combinedData.data() + realBytes);

        for (size_t i = 0; i < totalElements; ++i)
        {
            realPtr[i] = complexData[i].real();
            imagPtr[i] = complexData[i].imag();
        }
    }
    else if (type == helper::GetDataType<std::complex<double>>())
    {
        const std::complex<double> *complexData =
            reinterpret_cast<const std::complex<double> *>(dataIn);
        double *realPtr = reinterpret_cast<double *>(combinedData.data());
        double *imagPtr = reinterpret_cast<double *>(combinedData.data() + realBytes);

        for (size_t i = 0; i < totalElements; ++i)
        {
            realPtr[i] = complexData[i].real();
            imagPtr[i] = complexData[i].imag();
        }
    }
    // Compress real part
    size_t realCompressedSize = realBytes;
    void *realCompressedData = bufferOut + bufferOutOffset;

    mgard_x::compress(mgardDim, mgardType, mgardCount, tolerance, s, errorBoundType,
                      combinedData.data(), realCompressedData, realCompressedSize, config, true);
    // std::cout << "realCompressedSize=" << realCompressedSize << "\n";
    // Update the actual compressed size of real part in header
    *reinterpret_cast<size_t *>(bufferOut + realSizeOffset) = realCompressedSize;
    bufferOutOffset += realCompressedSize;

    // Compress imaginary part
    size_t imagCompressedSize = realBytes;
    void *imagCompressedData = bufferOut + bufferOutOffset;

    mgard_x::compress(mgardDim, mgardType, mgardCount, tolerance, s, errorBoundType,
                      combinedData.data() + realBytes, imagCompressedData, imagCompressedSize,
                      config, true);

    // std::cout << "imagCompressedSize=" << imagCompressedSize << "\n";
    // Update the actual compressed size of imaginary part in header
    *reinterpret_cast<size_t *>(bufferOut + imagSizeOffset) = imagCompressedSize;
    bufferOutOffset += imagCompressedSize;

    return bufferOutOffset;
}

size_t CompressMGARDComplex::GetHeaderSize() const { return headerSize; }

size_t CompressMGARDComplex::DecompressV1(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 0;

    const size_t ndims = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    Dims blockCount(ndims);
    for (size_t i = 0; i < ndims; ++i)
    {
        blockCount[i] = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    }
    const DataType type = GetParameter<DataType>(bufferIn, bufferInOffset);
    m_VersionInfo = " Data is compressed using MGARD Version " +
                    std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
                    std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
                    std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) +
                    ". Please make sure a compatible version is used for decompression.";

    const bool isCompressed = GetParameter<bool>(bufferIn, bufferInOffset);

    const size_t totalElements = helper::GetTotalSize(blockCount, 1);
    const size_t totalBytes = helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));

    size_t elementSize = 0;
    if (type == helper::GetDataType<std::complex<float>>())
    {
        elementSize = sizeof(float);
    }
    else if (type == helper::GetDataType<std::complex<double>>())
    {
        elementSize = sizeof(double);
    }

    const size_t realBytes = totalElements * elementSize;

    if (isCompressed)
    {
        try
        {
            // Get compressed sizes from header
            const size_t realCompressedSize =
                GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
            const size_t imagCompressedSize =
                GetParameter<size_t, size_t>(bufferIn, bufferInOffset);

            // Use std::vector<char> for unified data handling
            std::vector<char> combinedData(2 *
                                           realBytes); // Space for both real and imaginary parts
            if (type == helper::GetDataType<std::complex<float>>())
            {
                // Decompress real part
                void *realPartVoid = combinedData.data();
                mgard_x::decompress(bufferIn + bufferInOffset, realCompressedSize, realPartVoid,
                                    true);
                bufferInOffset += realCompressedSize;

                // Decompress imaginary part
                void *imagPartVoid = combinedData.data() + realBytes;
                mgard_x::decompress(bufferIn + bufferInOffset, imagCompressedSize, imagPartVoid,
                                    true);

                // Reconstruct complex data
                const float *realPtr = reinterpret_cast<const float *>(combinedData.data());
                const float *imagPtr =
                    reinterpret_cast<const float *>(combinedData.data() + realBytes);
                std::complex<float> *complexOut = reinterpret_cast<std::complex<float> *>(dataOut);

                for (size_t i = 0; i < totalElements; ++i)
                {
                    complexOut[i] = std::complex<float>(realPtr[i], imagPtr[i]);
                }
            }
            else if (type == helper::GetDataType<std::complex<double>>())
            {
                // Decompress real part
                void *realPartVoid = combinedData.data();
                mgard_x::decompress(bufferIn + bufferInOffset, realCompressedSize, realPartVoid,
                                    true);
                bufferInOffset += realCompressedSize;

                // Decompress imaginary part
                void *imagPartVoid = combinedData.data() + realBytes;
                mgard_x::decompress(bufferIn + bufferInOffset, imagCompressedSize, imagPartVoid,
                                    true);

                // Reconstruct complex data
                const double *realPtr = reinterpret_cast<const double *>(combinedData.data());
                const double *imagPtr =
                    reinterpret_cast<const double *>(combinedData.data() + realBytes);
                std::complex<double> *complexOut =
                    reinterpret_cast<std::complex<double> *>(dataOut);

                for (size_t i = 0; i < totalElements; ++i)
                {
                    complexOut[i] = std::complex<double>(realPtr[i], imagPtr[i]);
                }
            }
        }
        catch (...)
        {
            helper::Throw<std::runtime_error>("Operator", "CompressMGARDComplex", "DecompressV1",
                                              m_VersionInfo);
        }
        return totalBytes;
    }

    headerSize += bufferInOffset;
    return 0;
}

size_t CompressMGARDComplex::InverseOperate(const char *bufferIn, const size_t sizeIn,
                                            char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion = GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes
    headerSize = bufferInOffset;

    if (bufferVersion == 1)
    {
        return DecompressV1(bufferIn + bufferInOffset, sizeIn - bufferInOffset, dataOut);
    }
    else if (bufferVersion == 2)
    {
        // TODO: if a Version 2 mgard complex buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        helper::Throw<std::runtime_error>("Operator", "CompressMGARDComplex", "InverseOperate",
                                          "invalid mgard complex buffer version");
    }

    return 0;
}

bool CompressMGARDComplex::IsDataTypeValid(const DataType type) const
{
    if (type == DataType::DoubleComplex || type == DataType::FloatComplex)
    {
        return true;
    }
    return false;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2