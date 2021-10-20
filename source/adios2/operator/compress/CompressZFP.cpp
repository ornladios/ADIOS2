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

size_t CompressZFP::Compress(const char *dataIn, const Dims &blockStart,
                             const Dims &blockCount, const DataType type,
                             char *bufferOut, const Params &parameters)
{

    const uint8_t bufferVersion = 1;
    size_t bufferOutOffset = 0;

    // Universal operator metadata
    PutParameter(bufferOut, bufferOutOffset, OperatorType::ZFP);
    PutParameter(bufferOut, bufferOutOffset, bufferVersion);
    PutParameter(bufferOut, bufferOutOffset, static_cast<uint16_t>(0));
    // Universal operator metadata end

    const size_t ndims = blockCount.size();

    // zfp V1 metadata
    PutParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : blockCount)
    {
        PutParameter(bufferOut, bufferOutOffset, d);
    }
    PutParameter(bufferOut, bufferOutOffset, type);
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(ZFP_VERSION_MAJOR));
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(ZFP_VERSION_MINOR));
    PutParameter(bufferOut, bufferOutOffset,
                 static_cast<uint8_t>(ZFP_VERSION_PATCH));
    PutParameters(bufferOut, bufferOutOffset, parameters);
    // zfp V1 metadata end

    Dims convertedDims = ConvertDims(blockCount, type, 3);
    zfp_field *field = GetZFPField(dataIn, convertedDims, type);
    zfp_stream *stream = GetZFPStream(convertedDims, type, parameters);
    size_t maxSize = zfp_stream_maximum_size(stream, field);
    // associate bitstream
    bitstream *bitstream = stream_open(bufferOut + bufferOutOffset, maxSize);
    zfp_stream_set_bit_stream(stream, bitstream);
    zfp_stream_rewind(stream);

    size_t sizeOut = zfp_compress(stream, field);

    if (sizeOut == 0)
    {
        throw std::invalid_argument("ERROR: zfp failed, compressed buffer "
                                    "size is 0, in call to Compress");
    }

    bufferOutOffset += sizeOut;

    zfp_field_free(field);
    zfp_stream_close(stream);
    stream_close(bitstream);

    const size_t sizeIn =
        helper::GetTotalSize(blockCount, helper::GetDataTypeSize(type));
    if (sizeIn < bufferOutOffset)
    {
        std::cerr
            << "ADIOS2 zfp operator warning: compression buffer size larger "
               "than original data size. Consider disabling compression."
            << std::endl;
    }

    return bufferOutOffset;
}

size_t CompressZFP::DecompressV1(const char *bufferIn, const size_t sizeIn,
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
        " Data is compressed using ZFP Version " +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) + "." +
        std::to_string(GetParameter<uint8_t>(bufferIn, bufferInOffset)) +
        ". Please make sure a compatible version is used for decompression.";
    const Params parameters = GetParameters(bufferIn, bufferInOffset);

    Dims convertedDims = ConvertDims(blockCount, type, 3);

    zfp_field *field = GetZFPField(dataOut, convertedDims, type);
    zfp_stream *stream = GetZFPStream(convertedDims, type, parameters);

    // associate bitstream
    bitstream *bitstream = stream_open(
        const_cast<char *>(bufferIn + bufferInOffset), sizeIn - bufferInOffset);
    zfp_stream_set_bit_stream(stream, bitstream);
    zfp_stream_rewind(stream);

    int status = zfp_decompress(stream, field);

    if (!status)
    {
        throw std::runtime_error(
            "ERROR: zfp failed with status " + std::to_string(status) +
            ", in call to CompressZfp Decompress." + m_VersionInfo + "\n");
    }

    zfp_field_free(field);
    zfp_stream_close(stream);
    stream_close(bitstream);

    return helper::GetTotalSize(convertedDims, helper::GetDataTypeSize(type));
}

size_t CompressZFP::Decompress(const char *bufferIn, const size_t sizeIn,
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
        // TODO: if a Version 2 zfp buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        throw std::runtime_error("unknown zfp buffer version");
    }

    return 0;
}

bool CompressZFP::IsDataTypeValid(const DataType type) const
{
#define declare_type(T)                                                        \
    if (helper::GetDataType<T>() == type)                                      \
    {                                                                          \
        return true;                                                           \
    }
    ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type
    return false;
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
    else if (type == helper::GetDataType<std::complex<float>>())
    {
        zfpType = zfp_type_float;
    }
    else if (type == helper::GetDataType<std::complex<double>>())
    {
        zfpType = zfp_type_double;
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

zfp_field *CompressZFP::GetZFPField(const char *data, const Dims &dimensions,
                                    DataType type) const
{
    zfp_type zfpType = GetZfpType(type);
    zfp_field *field = nullptr;

    if (dimensions.size() == 1)
    {
        field = zfp_field_1d(const_cast<char *>(data), zfpType, dimensions[0]);
    }
    else if (dimensions.size() == 2)
    {
        field = zfp_field_2d(const_cast<char *>(data), zfpType, dimensions[0],
                             dimensions[1]);
    }
    else if (dimensions.size() == 3)
    {
        field = zfp_field_3d(const_cast<char *>(data), zfpType, dimensions[0],
                             dimensions[1], dimensions[2]);
    }
    else
    {
        throw std::invalid_argument(
            "ERROR: zfp_field* failed for data of type " + ToString(type) +
            ", only 1D, 2D and 3D dimensions are supported, from "
            "class CompressZfp." +
            m_VersionInfo + "\n");
    }

    if (field == nullptr)
    {
        throw std::invalid_argument(
            "ERROR: zfp_field_" + std::to_string(dimensions.size()) +
            "d failed for data of type " + ToString(type) +
            ", data might be corrupted, from class CompressZfp." +
            m_VersionInfo + "\n");
    }

    return field;
}

zfp_stream *CompressZFP::GetZFPStream(const Dims &dimensions, DataType type,
                                      const Params &parameters) const
{
    zfp_stream *stream = zfp_stream_open(NULL);

    auto itAccuracy = parameters.find("accuracy");
    const bool hasAccuracy = itAccuracy != parameters.end();

    auto itRate = parameters.find("rate");
    const bool hasRate = itRate != parameters.end();

    auto itPrecision = parameters.find("precision");
    const bool hasPrecision = itPrecision != parameters.end();

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
