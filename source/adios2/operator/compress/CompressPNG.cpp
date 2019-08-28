/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressPNG.cpp
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */
#include "CompressPNG.h"

#include <cstring> // std::memset

extern "C" {
#include <png.h>
}

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

const std::map<std::string, uint32_t> CompressPNG::m_ColorTypes = {
    {"PNG_COLOR_TYPE_GRAY", PNG_COLOR_TYPE_GRAY},
    {"PNG_COLOR_TYPE_PALETTE", PNG_COLOR_TYPE_PALETTE},
    {"PNG_COLOR_TYPE_RGB", PNG_COLOR_TYPE_RGB},
    {"PNG_COLOR_TYPE_RGB_ALPHA", PNG_COLOR_TYPE_RGB_ALPHA},
    {"PNG_COLOR_TYPE_GRAY_ALPHA", PNG_COLOR_TYPE_GRAY_ALPHA},
    {"PNG_COLOR_TYPE_RGBA", PNG_COLOR_TYPE_RGBA},
    {"PNG_COLOR_TYPE_GA", PNG_COLOR_TYPE_GA}};

const std::map<std::string, std::set<uint32_t>> CompressPNG::m_BitDepths = {
    {"PNG_COLOR_TYPE_GRAY", {1, 2, 4, 8, 16}},
    {"PNG_COLOR_TYPE_PALETTE", {1, 2, 4, 8}},
    {"PNG_COLOR_TYPE_RGB", {8, 16}},
    {"PNG_COLOR_TYPE_RGB_ALPHA", {8, 16}},
    {"PNG_COLOR_TYPE_GRAY_ALPHA", {8, 16}},
    {"PNG_COLOR_TYPE_RGBA", {8, 16}},
    {"PNG_COLOR_TYPE_GA", {8, 16}}};

// PUBLIC
CompressPNG::CompressPNG(const Params &parameters, const bool debugMode)
: Operator("png", parameters, debugMode)
{
}

size_t CompressPNG::Compress(const void *dataIn, const Dims &dimensions,
                             const size_t elementSize,
                             const std::string /*type*/, void *bufferOut,
                             const Params &parameters, Params &info) const
{
    auto lf_Write = [](png_structp png_ptr, png_bytep data, png_size_t length) {
        DestInfo *pDestInfo =
            reinterpret_cast<DestInfo *>(png_get_io_ptr(png_ptr));
        std::memcpy(pDestInfo->BufferOut + pDestInfo->Offset, data, length);
        pDestInfo->Offset += length;
    };

    const std::size_t ndims = dimensions.size();
    if (m_DebugMode)
    {
        if (ndims != 3 && ndims != 2)
        {
            throw std::invalid_argument(
                "ERROR: image number of dimensions " + std::to_string(ndims) +
                " is invalid, must be 2 {height,width*bytes_per_pixel} or 3"
                " {height,width,bytes_per_pixel]} , in call to ADIOS2 PNG "
                " compression\n");
        }
    }

    // defaults
    int compressionLevel = 1;
    int colorType = PNG_COLOR_TYPE_RGBA;
    int bitDepth = 8;
    std::string colorTypeStr = "PNG_COLOR_TYPE_RGBA";

    for (const auto &itParameter : parameters)
    {
        const std::string key = itParameter.first;
        const std::string value = itParameter.second;

        if (key == "compression_level")
        {
            compressionLevel = static_cast<int>(helper::StringTo<int32_t>(
                value, m_DebugMode, "when setting PNG level parameter\n"));
            if (m_DebugMode)
            {
                if (compressionLevel < 1 || compressionLevel > 9)
                {
                    throw std::invalid_argument(
                        "ERROR: compression_level must be an "
                        "integer between 1 (less "
                        "compression, less memory) and 9 "
                        "(more compression, more memory) inclusive, in call to "
                        "ADIOS2 PNG Compress\n");
                }
            }
        }
        else if (key == "color_type")
        {
            auto itColorType = m_ColorTypes.find(value);
            if (m_DebugMode)
            {
                if (itColorType == m_ColorTypes.end())
                {
                    throw std::invalid_argument(
                        "ERROR: invalid color_type, see PNG_COLOR_TYPE_* for "
                        "available types, in call to ADIOS2 PNG Compress\n");
                }
            }

            colorTypeStr = itColorType->first;
            colorType = itColorType->second;
        }
        else if (key == "bit_depth")
        {
            bitDepth = static_cast<int>(helper::StringTo<int32_t>(
                value, m_DebugMode, "when setting PNG bit_depth parameter\n"));
        }
    }

    if (m_DebugMode)
    {
        if (m_BitDepths.at(colorTypeStr)
                .count(static_cast<int32_t>(bitDepth)) == 0)
        {
            throw std::invalid_argument(
                "ERROR: bit_depth " + std::to_string(bitDepth) +
                " and color_type " + colorTypeStr +
                " combination is not allowed by libpng, in call to ADIOS2 PNG "
                "compression\n");
        }
    }

    png_structp pngWrite = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                   nullptr, nullptr, nullptr);
    png_infop pngInfo = png_create_info_struct(pngWrite);

    const uint32_t bytesPerPixel =
        ndims == 3 ? static_cast<uint32_t>(dimensions[2]) : elementSize;

    const uint32_t width = static_cast<uint32_t>(dimensions[1]);
    const uint32_t height = static_cast<uint32_t>(dimensions[0]);

    png_set_IHDR(pngWrite, pngInfo, width, height, bitDepth, colorType,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    if (m_DebugMode && setjmp(png_jmpbuf(pngWrite)))
    {
        throw std::invalid_argument(
            "ERROR: libpng detected an error in ADIOS2 PNG Compress\n");
    }

    png_set_compression_level(pngWrite, compressionLevel);

    // set the rows
    std::vector<uint8_t *> rows(height);
    for (size_t r = 0; r < height; ++r)
    {
        rows[r] = reinterpret_cast<uint8_t *>(const_cast<void *>(dataIn)) +
                  r * width * bytesPerPixel;
    }
    png_set_rows(pngWrite, pngInfo, rows.data());

    DestInfo destInfo;
    destInfo.BufferOut = reinterpret_cast<char *>(bufferOut);
    destInfo.Offset = 0;

    png_set_write_fn(pngWrite, &destInfo, lf_Write, nullptr);
    png_write_png(pngWrite, pngInfo, PNG_TRANSFORM_IDENTITY, nullptr);
    png_write_end(pngWrite, pngInfo);

    // const size_t compressedSize = png_get_compression_buffer_size(pngWrite);
    png_destroy_write_struct(&pngWrite, &pngInfo);
    return destInfo.Offset;
}

size_t CompressPNG::Decompress(const void *bufferIn, const size_t sizeIn,
                               void *dataOut, const size_t sizeOut,
                               Params &info) const
{
    png_image image;
    std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    int result = png_image_begin_read_from_memory(&image, bufferIn, sizeIn);

    if (m_DebugMode && result == 0)
    {
        throw std::runtime_error(
            "ERROR: png_image_begin_read_from_memory failed in call "
            "to ADIOS2 PNG Decompress\n");
    }

    // TODO might be needed from parameters?
    result = png_image_finish_read(&image, nullptr, dataOut, 0, nullptr);
    if (m_DebugMode && result == 0)
    {
        throw std::runtime_error(
            "ERROR: png_image_finish_read_from_memory failed in call "
            "to ADIOS2 PNG Decompress\n");
    }

    return sizeOut;
}

void CompressPNG::CheckStatus(const int status, const std::string hint) const {}

} // end namespace compress
} // end namespace core
} // end namespace adios2
