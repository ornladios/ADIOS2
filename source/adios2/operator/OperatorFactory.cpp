/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * OperatorFactory.cpp :
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

std::string OperatorTypeToString(const Operator::OperatorType type)
{
    switch (type)
    {
    case Operator::COMPRESS_BLOSC:
        return "blosc";
    case Operator::COMPRESS_BZIP2:
        return "bzip2";
    case Operator::COMPRESS_LIBPRESSIO:
        return "libpressio";
    case Operator::COMPRESS_MGARD:
        return "mgard";
    case Operator::COMPRESS_PNG:
        return "png";
    case Operator::COMPRESS_SIRIUS:
        return "sirius";
    case Operator::COMPRESS_SZ:
        return "sz";
    case Operator::COMPRESS_ZFP:
        return "zfp";
    default:
        return "null";
    }
}

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
    else if (typeLowerCase == "sirius")
    {
        ret = std::make_shared<compress::CompressSirius>(parameters);
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
    else if (typeLowerCase == "null")
    {
        ret = std::make_shared<compress::CompressNull>(parameters);
    }
    else
    {
        helper::Log("Operator", "OperatorFactory", "MakeOperator",
                    "ADIOS2 does not support " + typeLowerCase + " operation",
                    helper::EXCEPTION);
    }

    if (ret == nullptr)
    {
        helper::Log("Operator", "OperatorFactory", "MakeOperator",
                    "ADIOS2 didn't compile with " + typeLowerCase +
                        "library, operator not added",
                    helper::EXCEPTION);
    }

    return ret;
}

size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    Operator::OperatorType compressorType;
    std::memcpy(&compressorType, bufferIn, 1);
    auto op = MakeOperator(OperatorTypeToString(compressorType), Params());
    return op->InverseOperate(bufferIn, sizeIn, dataOut);
}

} // end namespace core
} // end namespace adios2
