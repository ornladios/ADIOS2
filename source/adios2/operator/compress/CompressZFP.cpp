/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressZFP.cpp
 *
 *  Created on: Jul 25, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "CompressZFP.h"
#include "adios2/helper/adiosFunctions.h"
#include <sstream>

namespace adios2
{
namespace core
{
namespace compress
{

CompressZFP::CompressZFP(const Params &parameters) : Operator("zfp", parameters)
{
}

size_t CompressZFP::DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                                    DataType type,
                                    const Params &parameters) const
{
    zfp_field *field = GetZFPField(dataIn, dimensions, type);
    zfp_stream *stream = GetZFPStream(dimensions, type, parameters);
    const size_t maxSize = zfp_stream_maximum_size(stream, field);
    zfp_field_free(field);
    zfp_stream_close(stream);
    return maxSize;
}

size_t CompressZFP::Compress(const void *dataIn, const Dims &dimensions,
                             const size_t elementSize, DataType type,
                             void *bufferOut, const Params &parameters,
                             Params &info) const
{

    zfp_field *field = GetZFPField(dataIn, dimensions, type);
    zfp_stream *stream = GetZFPStream(dimensions, type, parameters);
    size_t maxSize = zfp_stream_maximum_size(stream, field);
    // associate bitstream
    bitstream *bitstream = stream_open(bufferOut, maxSize);
    zfp_stream_set_bit_stream(stream, bitstream);
    zfp_stream_rewind(stream);

    size_t sizeOut = zfp_compress(stream, field);

    if (sizeOut == 0)
    {
        throw std::invalid_argument("ERROR: zfp failed, compressed buffer "
                                    "size is 0, in call to Compress");
    }

    zfp_field_free(field);
    zfp_stream_close(stream);
    stream_close(bitstream);
    return sizeOut;
}

size_t CompressZFP::Decompress(const void *bufferIn, const size_t sizeIn,
                               void *dataOut, const Dims &dimensions,
                               DataType type, const Params &parameters) const
{
    auto lf_GetTypeSize = [](const zfp_type zfpType) -> size_t {
        size_t size = 0;
        if (zfpType == zfp_type_int32 || zfpType == zfp_type_float)
        {
            size = 4;
        }
        else if (zfpType == zfp_type_int64 || zfpType == zfp_type_double)
        {
            size = 8;
        }
        return size;
    };

    zfp_field *field = GetZFPField(dataOut, dimensions, type);
    zfp_stream *stream = GetZFPStream(dimensions, type, parameters);

    // associate bitstream
    bitstream *bitstream = stream_open(const_cast<void *>(bufferIn), sizeIn);
    zfp_stream_set_bit_stream(stream, bitstream);
    zfp_stream_rewind(stream);

    int status = zfp_decompress(stream, field);

    if (!status)
    {
        throw std::invalid_argument("ERROR: zfp failed with status " +
                                    std::to_string(status) +
                                    ", in call to CompressZfp Decompress\n");
    }

    zfp_field_free(field);
    zfp_stream_close(stream);
    stream_close(bitstream);

    const size_t typeSizeBytes = lf_GetTypeSize(GetZfpType(type));
    const size_t dataSizeBytes =
        helper::GetTotalSize(dimensions) * typeSizeBytes;

    return dataSizeBytes;
}

// PRIVATE
zfp_type CompressZFP::GetZfpType(DataType type) const
{
    zfp_type zfpType = zfp_type_none;

    if (type == helper::GetDataType<double>())
    {
        zfpType = zfp_type_double;
    }
    else if (type == helper::GetDataType<float>())
    {
        zfpType = zfp_type_float;
    }
    else if (type == helper::GetDataType<int64_t>())
    {
        zfpType = zfp_type_int64;
    }
    else if (type == helper::GetDataType<int32_t>())
    {
        zfpType = zfp_type_int32;
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: type " + ToString(type) +
            " not supported by zfp, only "
            "signed int32_t, signed int64_t, float, and "
            "double types are acceptable, from class "
            "CompressZfp Transform\n");
    }

    return zfpType;
}

zfp_field *CompressZFP::GetZFPField(const void *data, const Dims &dimensions,
                                    DataType type) const
{
    auto lf_CheckField = [](const zfp_field *field,
                            const std::string zfpFieldFunction, DataType type) {
        if (field == nullptr || field == NULL)
        {
            throw std::invalid_argument(
                "ERROR: " + zfpFieldFunction + " failed for data of type " +
                ToString(type) +
                ", data pointer might be corrupted, from "
                "class CompressZfp Transform\n");
        }
    };

    zfp_type zfpType = GetZfpType(type);
    zfp_field *field = nullptr;

    if (dimensions.size() == 1)
    {
        field = zfp_field_1d(const_cast<void *>(data), zfpType, dimensions[0]);
        lf_CheckField(field, "zfp_field_1d", type);
    }
    else if (dimensions.size() == 2)
    {
        field = zfp_field_2d(const_cast<void *>(data), zfpType, dimensions[0],
                             dimensions[1]);
        lf_CheckField(field, "zfp_field_2d", type);
    }
    else if (dimensions.size() == 3)
    {
        field = zfp_field_3d(const_cast<void *>(data), zfpType, dimensions[0],
                             dimensions[1], dimensions[2]);
        lf_CheckField(field, "zfp_field_3d", type);
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: zfp_field* failed for data of type " + ToString(type) +
            ", only 1D, 2D and 3D dimensions are supported, from "
            "class CompressZfp Transform\n");
    }

    return field;
}

zfp_stream *CompressZFP::GetZFPStream(const Dims &dimensions, DataType type,
                                      const Params &parameters) const
{
    auto lf_HasKey = [](Params::const_iterator itKey,
                        const Params &parameters) -> bool {
        bool hasKey = false;
        if (itKey != parameters.end())
        {
            hasKey = true;
        }
        return hasKey;
    };

    zfp_stream *stream = zfp_stream_open(NULL);

    auto itAccuracy = parameters.find("accuracy");
    const bool hasAccuracy = lf_HasKey(itAccuracy, parameters);

    auto itRate = parameters.find("rate");
    const bool hasRate = lf_HasKey(itRate, parameters);

    auto itPrecision = parameters.find("precision");
    const bool hasPrecision = lf_HasKey(itPrecision, parameters);

    if ((hasAccuracy && hasRate) || (hasAccuracy && hasPrecision) ||
        (hasRate && hasPrecision) || !(hasAccuracy || hasRate || hasPrecision))
    {
        std::ostringstream oss;
        oss << "\nError: Requisite parameters to zfp not found.";
        oss << " The key must be one and only one of 'accuracy', 'rate', "
               "or 'precision'.";
        oss << " The key and value provided are ";
        for (auto &p : parameters)
        {
            oss << "(" << p.first << ", " << p.second << ").";
        }
        throw std::invalid_argument(oss.str());
    }

    if (hasAccuracy)
    {
        const double accuracy = helper::StringTo<double>(
            itAccuracy->second, "setting accuracy in call to CompressZfp\n");

        zfp_stream_set_accuracy(stream, accuracy);
    }
    else if (hasRate)
    {
        const double rate = helper::StringTo<double>(
            itRate->second, "setting Rate in call to CompressZfp\n");
        // TODO support last argument write random access?
        zfp_stream_set_rate(stream, rate, GetZfpType(type),
                            static_cast<unsigned int>(dimensions.size()), 0);
    }
    else if (hasPrecision)
    {
        const unsigned int precision =
            static_cast<unsigned int>(helper::StringTo<uint32_t>(
                itPrecision->second,
                "setting Precision in call to CompressZfp\n"));
        zfp_stream_set_precision(stream, precision);
    }

    return stream;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
