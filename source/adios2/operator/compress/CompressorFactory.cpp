/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressorFactory.cpp :
 *
 *  Created on: Sep 29, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "CompressorFactory.h"
#include "adios2/core/Operator.h"
#include "adios2/helper/adiosFunctions.h"
#include <numeric>

namespace adios2
{
namespace core
{
namespace compress
{

bool IsCompressionAvailable(const std::string &method, DataType type,
                            const Dims &count)
{
    if (method == "zfp")
    {
        if (type == helper::GetDataType<int32_t>() ||
            type == helper::GetDataType<int64_t>() ||
            type == helper::GetDataType<float>() ||
            type == helper::GetDataType<double>())
        {
            if (count.size() <= 3)
            {
                return true;
            }
        }
    }
    else if (method == "sz")
    {
        if (type == helper::GetDataType<float>() ||
            type == helper::GetDataType<double>())
        {
            if (count.size() <= 5)
            {
                size_t elements =
                    std::accumulate(count.begin(), count.end(), 1ull,
                                    std::multiplies<size_t>());
                if (elements >= 10)
                {
                    return true;
                }
            }
        }
    }
    else if (method == "bzip2")
    {
        if (type == helper::GetDataType<int32_t>() ||
            type == helper::GetDataType<int64_t>() ||
            type == helper::GetDataType<float>() ||
            type == helper::GetDataType<double>())
        {
            return true;
        }
    }
    else if (method == "mgard")
    {
        return true;
    }
    return false;
}

size_t Compress(const char *dataIn, const Dims &blockStart,
                const Dims &blockCount, const DataType dataType,
                char *bufferOut, const Params &parameters,
                const std::string &compressorType)
{
    if (compressorType == "blosc")
    {
#ifdef ADIOS2_HAVE_BLOSC
        CompressBlosc c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with BLOSC",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "bzip2")
    {
#ifdef ADIOS2_HAVE_BZIP2
        CompressBZIP2 c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with BZIP2",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "libpressio")
    {
#ifdef ADIOS2_HAVE_LIBPRESSIO
        CompressLibPressio c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with LibPressio",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "mgard")
    {
#ifdef ADIOS2_HAVE_MGARD
        CompressMGARD c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with MGARD",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "png")
    {
#ifdef ADIOS2_HAVE_PNG
        CompressPNG c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with PNG",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "sirius")
    {
#ifdef ADIOS2_HAVE_MHS
        CompressSirius c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't enable MHS",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "sz")
    {
#ifdef ADIOS2_HAVE_SZ
        CompressSZ c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with SZ",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == "zfp")
    {
#ifdef ADIOS2_HAVE_ZFP
        CompressZFP c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with ZFP",
                    helper::LogMode::EXCEPTION);
#endif
    }
    return 0;
}

size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    Operator::OperatorType compressorType;
    std::memcpy(&compressorType, bufferIn, 1);

    if (compressorType == Operator::OperatorType::COMPRESS_BLOSC)
    {
#ifdef ADIOS2_HAVE_BLOSC
        compress::CompressBlosc op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with BLOSC",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_BZIP2)
    {
#ifdef ADIOS2_HAVE_BZIP2
        compress::CompressBZIP2 op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with BZIP2",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_LIBPRESSIO)
    {
#ifdef ADIOS2_HAVE_LIBPRESSIO
        compress::CompressLibPressio op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with LibPressio",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_MGARD)
    {
#ifdef ADIOS2_HAVE_MGARD
        compress::CompressMGARD op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with MGARD",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_PNG)
    {
#ifdef ADIOS2_HAVE_PNG
        compress::CompressPNG op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with PNG",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_SIRIUS)
    {
#ifdef ADIOS2_HAVE_MHS
        compress::CompressSirius op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't enable MHS",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_SZ)
    {
#ifdef ADIOS2_HAVE_SZ
        compress::CompressSZ op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with SZ",
                    helper::LogMode::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::OperatorType::COMPRESS_ZFP)
    {
#ifdef ADIOS2_HAVE_ZFP
        compress::CompressZFP op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 installation didn't compile with ZFP",
                    helper::LogMode::EXCEPTION);
#endif
    }

    return 0;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
