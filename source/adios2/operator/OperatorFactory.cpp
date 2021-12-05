/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressorFactory.cpp :
 *
 *  Created on: Sep 29, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "OperatorFactory.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/operator/compress/CompressNull.h"
#include <numeric>

#ifdef ADIOS2_HAVE_BLOSC
#include "adios2/operator/compress/CompressBlosc.h"
#endif

#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZIP2.h"
#endif

#ifdef ADIOS2_HAVE_LIBPRESSIO
#include "adios2/operator/compress/CompressLibPressio.h"
#endif

#ifdef ADIOS2_HAVE_MGARD
#include "adios2/operator/compress/CompressMGARD.h"
#endif

#ifdef ADIOS2_HAVE_PNG
#include "adios2/operator/compress/CompressPNG.h"
#endif

#ifdef ADIOS2_HAVE_MHS
#include "adios2/operator/compress/CompressSirius.h"
#endif

#ifdef ADIOS2_HAVE_SZ
#include "adios2/operator/compress/CompressSZ.h"
#endif

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZFP.h"
#endif

namespace adios2
{
namespace core
{

std::shared_ptr<Operator> MakeOperator(const std::string &type,
                                       const Params &parameters)
{
    std::shared_ptr<Operator> ret = nullptr;

    const std::string typeLowerCase = helper::LowerCase(type);

    if (typeLowerCase == "blosc")
    {
#ifdef ADIOS2_HAVE_BLOSC
        ret = std::make_shared<compress::CompressBlosc>(parameters);
#endif
    }
    else if (typeLowerCase == "bzip2")
    {
#ifdef ADIOS2_HAVE_BZIP2
        ret = std::make_shared<compress::CompressBZIP2>(parameters);
#endif
    }
    else if (typeLowerCase == "libpressio")
    {
#ifdef ADIOS2_HAVE_LIBPRESSIO
        ret = std::make_shared<compress::CompressLibPressio>(parameters);
#endif
    }
    else if (typeLowerCase == "mgard")
    {
#ifdef ADIOS2_HAVE_MGARD
        ret = std::make_shared<compress::CompressMGARD>(parameters);
#endif
    }
    else if (typeLowerCase == "png")
    {
#ifdef ADIOS2_HAVE_PNG
        ret = std::make_shared<compress::CompressPNG>(parameters);
#endif
    }
    else if (typeLowerCase == "sz")
    {
#ifdef ADIOS2_HAVE_SZ
        ret = std::make_shared<compress::CompressSZ>(parameters);
#endif
    }
    else if (typeLowerCase == "zfp")
    {
#ifdef ADIOS2_HAVE_ZFP
        ret = std::make_shared<compress::CompressZFP>(parameters);
#endif
    }
    else
    {
        helper::Log("Operator", "CompressorFactory", "MakeOperator",
                    "ADIOS2 does not support " + typeLowerCase + " operation",
                    helper::EXCEPTION);
    }

    if (ret == nullptr)
    {
        helper::Log("Operator", "CompressorFactory", "MakeOperator",
                    "ADIOS2 didn't compile with " + typeLowerCase +
                        "library, operator not added",
                    helper::EXCEPTION);
    }

    return ret;
}

size_t Compress(const char *dataIn, const Dims &blockStart,
                const Dims &blockCount, const DataType dataType,
                char *bufferOut, const Params &parameters,
                const std::string &compressorType)
{
    size_t ret = 0;
    try
    {
        if (compressorType == "blosc")
        {
#ifdef ADIOS2_HAVE_BLOSC
            compress::CompressBlosc c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log(
                "Operator", "CompressorFactory", "Compress",
                "ADIOS2 didn't compile with BLOSC, compression disabled",
                helper::EXCEPTION);
#endif
        }
        else if (compressorType == "bzip2")
        {
#ifdef ADIOS2_HAVE_BZIP2
            compress::CompressBZIP2 c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log(
                "Operator", "CompressorFactory", "Compress",
                "ADIOS2 didn't compile with BZIP2, compression disabled",
                helper::EXCEPTION);
#endif
        }
        else if (compressorType == "libpressio")
        {
#ifdef ADIOS2_HAVE_LIBPRESSIO
            compress::CompressLibPressio c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log(
                "Operator", "CompressorFactory", "Compress",
                "ADIOS2 didn't compile with LibPressio, compression disabled",
                helper::EXCEPTION);
#endif
        }
        else if (compressorType == "mgard")
        {
#ifdef ADIOS2_HAVE_MGARD
            compress::CompressMGARD c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log(
                "Operator", "CompressorFactory", "Compress",
                "ADIOS2 didn't compile with MGARD, compression disabled",
                helper::EXCEPTION);
#endif
        }
        else if (compressorType == "png")
        {
#ifdef ADIOS2_HAVE_PNG
            compress::CompressPNG c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log("Operator", "CompressorFactory", "Compress",
                        "ADIOS2 didn't compile with PNG, compression disabled",
                        helper::EXCEPTION);
#endif
        }
        else if (compressorType == "sirius")
        {
#ifdef ADIOS2_HAVE_MHS
            compress::CompressSirius c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log("Operator", "CompressorFactory", "Compress",
                        "ADIOS2 didn't enable MHS, compression disabled",
                        helper::EXCEPTION);
#endif
        }
        else if (compressorType == "sz")
        {
#ifdef ADIOS2_HAVE_SZ
            compress::CompressSZ c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log("Operator", "CompressorFactory", "Compress",
                        "ADIOS2 didn't compile with SZ, compression disabled",
                        helper::EXCEPTION);
#endif
        }
        else if (compressorType == "zfp")
        {
#ifdef ADIOS2_HAVE_ZFP
            compress::CompressZFP c({});
            ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                            parameters);
#else
            helper::Log("Operator", "CompressorFactory", "Compress",
                        "ADIOS2 didn't compile with ZFP, compression disabled",
                        helper::EXCEPTION);
#endif
        }
        if (ret == 0)
        {
            helper::Log(
                "Operator", "CompressorFactory", "Compress",
                "compressor " + compressorType +
                    " failed with returned buffer size 0, compression disabled",
                helper::EXCEPTION);
        }
        else if (ret > helper::GetTotalSize(blockCount,
                                            helper::GetDataTypeSize(dataType)))
        {
            helper::Log("Operator", "CompressorFactory", "Compress",
                        "compressor " + compressorType +
                            " produced buffer larger than uncompressed data, "
                            "compression disabled",
                        helper::EXCEPTION);
        }
    }
    catch (std::exception &e)
    {
        helper::Log("Operator", "CompressorFactory", "Compress", e.what(),
                    helper::WARNING);
        compress::CompressNull c({});
        ret = c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                        parameters);
    }
    return ret;
}

size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    Operator::OperatorType compressorType;
    std::memcpy(&compressorType, bufferIn, 1);

    if (compressorType == Operator::COMPRESS_BLOSC)
    {
#ifdef ADIOS2_HAVE_BLOSC
        compress::CompressBlosc op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with BLOSC", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_BZIP2)
    {
#ifdef ADIOS2_HAVE_BZIP2
        compress::CompressBZIP2 op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with BZIP2", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_LIBPRESSIO)
    {
#ifdef ADIOS2_HAVE_LIBPRESSIO
        compress::CompressLibPressio op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with LibPressio", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_MGARD)
    {
#ifdef ADIOS2_HAVE_MGARD
        compress::CompressMGARD op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with MGARD", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_PNG)
    {
#ifdef ADIOS2_HAVE_PNG
        compress::CompressPNG op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with PNG", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_SIRIUS)
    {
#ifdef ADIOS2_HAVE_MHS
        compress::CompressSirius op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't enable MHS", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_SZ)
    {
#ifdef ADIOS2_HAVE_SZ
        compress::CompressSZ op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with SZ", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_ZFP)
    {
#ifdef ADIOS2_HAVE_ZFP
        compress::CompressZFP op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        helper::Log("Operator", "CompressorFactory", "Compress",
                    "ADIOS2 didn't compile with ZFP", helper::EXCEPTION);
#endif
    }
    else if (compressorType == Operator::COMPRESS_NULL)
    {
        compress::CompressNull op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
    }

    return 0;
}

} // end namespace core
} // end namespace adios2
