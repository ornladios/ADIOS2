/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressBigWhoop.cpp
 *
 *  Created on: Jan 26, 2024
 *      Author: Gregor Weiss gregor.weiss@hlrs.de
 */
#include "CompressBigWhoop.h"
#include "adios2/helper/adiosFunctions.h"
#include <bwc.h>
#include <sstream>

namespace adios2
{
namespace core
{
namespace compress
{

/**
 * Returns a adios2::dims object that contains targetDims entries. If the input
 * dimensionality is smaller the additional dimensions are set to defaultDimSize.
 * Else the entries at the front are multiplied and collapsed.
 * @param dimension
 * @param type
 * @param targetDims
 * @param enforceDims
 * @return refined dimensions
 */
Dims ConvertBwcDims(const Dims &dimensions, const DataType type, const size_t targetDims,
                    const bool enforceDims, const size_t defaultDimSize = 1);

/**
 * Returns BWC supported bwc_precision based on adios string type
 * @param type adios type as string, see GetDataType<T> in helper/adiosType.inl
 * @return bwc_precision
 */
bwc_precision GetBWCType(DataType type);

CompressBigWhoop::CompressBigWhoop(const Params &parameters)
: Operator("bigwhoop", COMPRESS_BIGWHOOP, "compress", parameters)
{
}

size_t CompressBigWhoop::Operate(const char *dataIn, const Dims &blockStart, const Dims &blockCount,
                                 const DataType type, char *bufferOut)
{
    const uint8_t bufferVersion = 1;
    size_t bufferOutOffset = 0;

    MakeCommonHeader(bufferOut, bufferOutOffset, bufferVersion);

    const size_t ndims = blockCount.size();

    // bwc V1 metadata
    PutParameter(bufferOut, bufferOutOffset, ndims);
    for (const auto &d : blockCount)
    {
        PutParameter(bufferOut, bufferOutOffset, d);
    }
    PutParameter(bufferOut, bufferOutOffset, type);
    // bwc V1 metadata end

    Dims convertedDims = ConvertBwcDims(blockCount, type, 5, true, 1);

    // Allocate BigWhoop coder
    bwc_precision bwcType = GetBWCType(type);
    bwc_codec *coder = bwc_alloc_coder(convertedDims[0], convertedDims[1], convertedDims[2],
                                       convertedDims[3], convertedDims[4], bwcType);
    if (coder == nullptr)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressBigWhoop", "Operate",
                                          "BigWhoop failed to make coder codec");
    }

    // Rate control
    std::string rate = "32";
    auto itRate = m_Parameters.find("rate");
    const bool hasRate = itRate != m_Parameters.end();
    rate = hasRate ? itRate->second : rate;

    // Quantization setting
    auto itQM = m_Parameters.find("qm");
    const bool hasQM = itQM != m_Parameters.end();
    if (hasQM)
    {
        const int QM =
            helper::StringTo<int>(itQM->second, "setting 'qm' in call to CompressBigWhoop\n");
        bwc_set_qm(coder, QM);
    }

    // Decomposition setting
    auto itTile = m_Parameters.find("tile");
    const bool hasTile = itTile != m_Parameters.end();
    if (hasTile)
    {
        const int tile =
            helper::StringTo<int>(itTile->second, "setting 'tile' in call to CompressBigWhoop\n");
        bwc_set_tiles(coder, tile, tile, tile, tile, bwc_tile_sizeof);
    }
    auto itPrecincts = m_Parameters.find("precincts");
    const bool hasPrecincts = itPrecincts != m_Parameters.end();
    if (hasPrecincts)
    {
        const int precincts = helper::StringTo<int>(
            itPrecincts->second, "setting 'precincts' in call to CompressBigWhoop\n");
        bwc_set_precincts(coder, precincts, precincts, precincts, precincts);
    }
    auto itCB = m_Parameters.find("codeblocks");
    const bool hasCB = itCB != m_Parameters.end();
    if (hasCB)
    {
        const int cb = helper::StringTo<int>(itCB->second,
                                             "setting 'codeblocks' in call to CompressBigWhoop\n");
        bwc_set_codeblocks(coder, cb, cb, cb, cb);
    }
    auto itDecomp = m_Parameters.find("decomposition");
    const bool hasDecomp = itDecomp != m_Parameters.end();
    if (hasDecomp)
    {
        const int decomp = helper::StringTo<int>(
            itDecomp->second, "setting 'decomposition' in call to CompressBigWhoop\n");
        bwc_set_decomp(coder, decomp, decomp, decomp, decomp);
    }

    // Compression
    bwc_stream *stream =
        bwc_init_stream(const_cast<char *>(dataIn), bufferOut + bufferOutOffset, comp);
    bwc_create_compression(coder, stream, const_cast<char *>(rate.data()));
    size_t sizeOut = (size_t)bwc_compress(coder, stream);

    if (sizeOut == 0)
    {
        helper::Throw<std::runtime_error>("Operator", "CompressBigWhoop", "Operate(Compress)",
                                          "BigWhoop failed, compressed buffer size is 0");
    }

    bufferOutOffset += sizeOut;

    free(stream);
    bwc_free_codec(coder);

    return bufferOutOffset;
}

size_t CompressBigWhoop::InverseOperate(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    size_t bufferInOffset = 1; // skip operator type
    const uint8_t bufferVersion = GetParameter<uint8_t>(bufferIn, bufferInOffset);
    bufferInOffset += 2; // skip two reserved bytes

    if (bufferVersion == 1)
    {
        return DecompressV1(bufferIn + bufferInOffset, sizeIn - bufferInOffset, dataOut);
    }
    else if (bufferVersion == 2)
    {
        // TODO: if a Version 2 buffer is being implemented, put it here
        // and keep the DecompressV1 routine for backward compatibility
    }
    else
    {
        helper::Throw<std::runtime_error>("Operator", "CompressBigWhoop", "InverseOperate",
                                          "invalid BigWhoop buffer version" +
                                              std::to_string(bufferVersion));
    }

    return 0;
}

bool CompressBigWhoop::IsDataTypeValid(const DataType type) const
{
    if (type == DataType::Float || type == DataType::Double || type == DataType::FloatComplex ||
        type == DataType::DoubleComplex)
    {
        return true;
    }
    return false;
}

// PRIVATE

size_t CompressBigWhoop::DecompressV1(const char *bufferIn, const size_t sizeIn, char *dataOut)
{
    // Do NOT remove even if the buffer version is updated. Data might be still
    // in legacy formats. This function must be kept for backward compatibility.
    // If a newer buffer format is implemented, create another function, e.g.
    // DecompressV2 and keep this function for decompressing legacy data.

    size_t bufferInOffset = 0;

    const size_t ndims = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    Dims blockCount(ndims);
    for (size_t i = 0; i < ndims; ++i)
    {
        blockCount[i] = GetParameter<size_t, size_t>(bufferIn, bufferInOffset);
    }
    const DataType type = GetParameter<DataType>(bufferIn, bufferInOffset);

    Dims convertedDims = ConvertBwcDims(blockCount, type, 5, true, 1);

    // Allocate BigWhoop decoder
    bwc_codec *decoder = bwc_alloc_decoder();

    // Decompression
    bwc_stream *stream =
        bwc_init_stream(const_cast<char *>(bufferIn) + bufferInOffset, dataOut, decomp);
    bwc_create_decompression(decoder, stream, 0);
    bwc_decompress(decoder, stream);

    free(stream);
    bwc_free_codec(decoder);

    return helper::GetTotalSize(convertedDims, helper::GetDataTypeSize(type));
}

Dims ConvertBwcDims(const Dims &dimensions, const DataType type, const size_t targetDims,
                    const bool enforceDims, const size_t defaultDimSize)
{
    if (targetDims < 1)
    {
        helper::Throw<std::invalid_argument>("Core", "Operator", "ConvertBwcDims",
                                             "only accepts targetDims > 0");
    }

    Dims ret = dimensions;

    while (ret.size() > targetDims)
    {
        ret[1] *= ret[0];
        ret.erase(ret.begin());
    }

    while (enforceDims && ret.size() < targetDims)
    {
        ret.push_back(defaultDimSize);
    }

    if (type == helper::GetDataType<std::complex<float>>() ||
        type == helper::GetDataType<std::complex<double>>())
    {
        ret.back() *= 2;
    }
    return ret;
}

bwc_precision GetBWCType(DataType type)
{
    bwc_precision bwcType; //= bwc_precision_none;

    if (type == helper::GetDataType<double>())
    {
        bwcType = bwc_precision_double;
    }
    else if (type == helper::GetDataType<float>())
    {
        bwcType = bwc_precision_single;
    }
    else if (type == helper::GetDataType<std::complex<float>>())
    {
        bwcType = bwc_precision_single;
    }
    else if (type == helper::GetDataType<std::complex<double>>())
    {
        bwcType = bwc_precision_double;
    }
    else
    {
        helper::Throw<std::invalid_argument>("Operator", "CompressBWC", "GetBWCType",
                                             "invalid data type " + ToString(type));
    }

    return bwcType;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
