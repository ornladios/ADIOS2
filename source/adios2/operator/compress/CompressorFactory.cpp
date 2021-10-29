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

namespace adios2
{
namespace core
{
namespace compress
{
size_t CompressorFactory::Compress(const char *dataIn, const Dims &blockStart,
                                   const Dims &blockCount,
                                   const DataType dataType, char *bufferOut,
                                   const Params &parameters,
                                   const std::string &compressorType)
{

    if (compressorType == "blosc")
    {
#ifdef ADIOS2_HAVE_BLOSC
        CompressBlosc c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with BLOSC, can't use compressor\n");
#endif
    }
    else if (compressorType == "bzip2")
    {
#ifdef ADIOS2_HAVE_BZIP2
        CompressBZIP2 c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with BZIP2, can't use compressor\n");
#endif
    }
    else if (compressorType == "libpressio")
    {
#ifdef ADIOS2_HAVE_LIBPRESSIO
        CompressLibPressio c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with LibPressio, can't use compressor\n");
#endif
    }
    else if (compressorType == "mgard")
    {
#ifdef ADIOS2_HAVE_MGARD
        CompressMGARD c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with MGARD, can't use compressor\n");
#endif
    }
    else if (compressorType == "png")
    {
#ifdef ADIOS2_HAVE_PNG
        CompressPNG c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with PNG, can't use compressor\n");
#endif
    }
    else if (compressorType == "sirius")
    {
#ifdef ADIOS2_HAVE_MHS
        CompressSirius c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with MHS, can't use Sirius compressor\n");
#endif
    }
    else if (compressorType == "sz")
    {
#ifdef ADIOS2_HAVE_SZ
        CompressSZ c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with SZ, can't use compressor\n");
#endif
    }
    else if (compressorType == "zfp")
    {
#ifdef ADIOS2_HAVE_ZFP
        CompressZFP c({});
        return c.Operate(dataIn, blockStart, blockCount, dataType, bufferOut,
                         parameters);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with ZFP, can't use compressor\n");
#endif
    }
    return 0;
}

size_t CompressorFactory::Decompress(const char *bufferIn, const size_t sizeIn,
                                     char *dataOut)
{
    Operator::OperatorType compressorType;
    std::memcpy(&compressorType, bufferIn, 1);

    if (compressorType == Operator::OperatorType::BLOSC)
    {
#ifdef ADIOS2_HAVE_BLOSC
        compress::CompressBlosc op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with BLOSC, can't use compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::BZIP2)
    {
#ifdef ADIOS2_HAVE_BZIP2
        compress::CompressBZIP2 op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with BZIP2, can't use compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::LIBPRESSIO)
    {
#ifdef ADIOS2_HAVE_LIBPRESSIO
        compress::CompressLibPressio op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with LibPressio, can't use compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::MGARD)
    {
#ifdef ADIOS2_HAVE_MGARD
        compress::CompressMGARD op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with MGARD, can't use compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::PNG)
    {
#ifdef ADIOS2_HAVE_PNG
        compress::CompressPNG op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with PNG, can't use compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::SIRIUS)
    {
#ifdef ADIOS2_HAVE_MHS
        compress::CompressSirius op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with MHS, can't use Sirius compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::Sz)
    {
#ifdef ADIOS2_HAVE_SZ
        compress::CompressSZ op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with SZ, can't use compressor\n");
#endif
    }
    else if (compressorType == Operator::OperatorType::ZFP)
    {
#ifdef ADIOS2_HAVE_ZFP
        compress::CompressZFP op({});
        return op.InverseOperate(bufferIn, sizeIn, dataOut);
#else
        throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                                 "with ZFP, can't use compressor\n");
#endif
    }

    return 0;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
