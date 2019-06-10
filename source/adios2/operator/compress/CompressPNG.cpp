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

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

const std::map<std::string, uint32_t> CompressPNG::m_Formats = {
    {"PNG_FORMAT_FLAG_ALPHA", PNG_FORMAT_FLAG_ALPHA},
    {"PNG_FORMAT_FLAG_COLOR", PNG_FORMAT_FLAG_COLOR},
    {"PNG_FORMAT_FLAG_LINEAR", PNG_FORMAT_FLAG_LINEAR},
    {"PNG_FORMAT_FLAG_COLORMAP", PNG_FORMAT_FLAG_COLORMAP},
#ifdef PNG_FORMAT_BGR_SUPPORTED
    {"PNG_FORMAT_FLAG_BGR", PNG_FORMAT_FLAG_BGR},
#endif
#ifdef PNG_FORMAT_AFIRST_SUPPORTED
    {"PNG_FORMAT_FLAG_AFIRST", PNG_FORMAT_FLAG_AFIRST},
#endif
    {"PNG_FORMAT_FLAG_ASSOCIATED_ALPHA", PNG_FORMAT_FLAG_ASSOCIATED_ALPHA},
    {"PNG_FORMAT_GRAY", PNG_FORMAT_GRAY},
    {"PNG_FORMAT_GA", PNG_FORMAT_GA},
    {"PNG_FORMAT_AG", PNG_FORMAT_AG},
    {"PNG_FORMAT_RGB", PNG_FORMAT_RGB},
    {"PNG_FORMAT_BGR", PNG_FORMAT_BGR},
    {"PNG_FORMAT_RGBA", PNG_FORMAT_RGBA},
    {"PNG_FORMAT_ARGB", PNG_FORMAT_ARGB},
    {"PNG_FORMAT_BGRA", PNG_FORMAT_BGRA},
    {"PNG_FORMAT_ABGR", PNG_FORMAT_ABGR},
    {"PNG_FORMAT_LINEAR_Y", PNG_FORMAT_LINEAR_Y},
    {"PNG_FORMAT_LINEAR_Y_ALPHA", PNG_FORMAT_LINEAR_Y_ALPHA},
    {"PNG_FORMAT_LINEAR_RBG_ALPHA", PNG_FORMAT_LINEAR_RGB_ALPHA},
    {"PNG_FORMAT_RGB_COLORMAP", PNG_FORMAT_RGB_COLORMAP},
    {"PNG_FORMAT_BGR_COLORMAP", PNG_FORMAT_BGR_COLORMAP},
    {"PNG_FORMAT_ARGB_COLORMAP", PNG_FORMAT_ARGB_COLORMAP},
    {"PNG_FORMAT_BGRA_COLORMAP", PNG_FORMAT_BGRA_COLORMAP},
    {"PNG_FORMAT_ABGR_COLORMAP", PNG_FORMAT_ABGR_COLORMAP},
};

// PUBLIC
CompressPNG::CompressPNG(const Params &parameters, const bool debugMode)
: Operator("png", parameters, debugMode)
{
}

size_t CompressPNG::DoBufferMaxSize(const void *dataIn, const Dims &dimensions,
                                    const std::string /*type*/,
                                    const Params &parameters) const
{
    png_image image = GetPNGImage(dimensions, parameters);

    size_t size;
    const bool result = png_image_write_to_memory(&image, nullptr, &size, 0,
                                                  dataIn, 0, nullptr);
    if (m_DebugMode && !result)
    {
        throw std::runtime_error("ERROR: could not get buffer size " +
                                 std::string(image.message) +
                                 ", in call to ADIOS2 PNG compression\n");
    }
    return size;
}

size_t CompressPNG::Compress(const void *dataIn, const Dims &dimensions,
                             const size_t /*elementSize*/,
                             const std::string /*type*/, void *bufferOut,
                             const Params &parameters, Params &info) const
{
    png_image image = GetPNGImage(dimensions, parameters);

    size_t size;
    const bool result = png_image_write_to_memory(&image, bufferOut, &size, 0,
                                                  dataIn, 0, nullptr);
    if (m_DebugMode && !result)
    {
        throw std::runtime_error("ERROR: could not compress " +
                                 std::string(image.message) +
                                 ", in call to ADIOS2 PNG compression\n");
    }
    return size;
}

size_t CompressPNG::Decompress(const void *bufferIn, const size_t sizeIn,
                               void *dataOut, const size_t sizeOut,
                               Params &info) const
{
    png_image image = GetPNGImage();

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

png_image CompressPNG::GetPNGImage(const Dims &dimensions,
                                   const Params &parameters) const
{
    png_image image;
    std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    // for reading
    if (dimensions.empty() && parameters.empty())
    {
        return image;
    }

    // defaults
    image.format = PNG_FORMAT_BGR;
    image.colormap_entries = 256;

    if (m_DebugMode && dimensions.size() != 2)
    {
        throw std::invalid_argument(
            "ERROR: image variable number of dimensions must be 2, in call to "
            "ADIOS2 PNG compression\n");
    }

    image.height = static_cast<uint32_t>(dimensions[0]);
    image.width = static_cast<uint32_t>(dimensions[1]);

    // TODO need a safe cast function
    for (auto it = parameters.begin(); it != parameters.end(); it++)
    {
        const std::string key = it->first;
        const std::string value = it->second;

        // set format
        if (key == "format")
        {
            auto itFormat = m_Formats.find(value);
            if (m_DebugMode && itFormat == m_Formats.end())
            {
                throw std::invalid_argument(
                    "ERROR: invalid format value " + value +
                    ", check png documentation for acceptable formats "
                    "PNG_FORMAT_*, in call to ADIOS2 PNG compressor\n");
            }

            image.format = itFormat->second;
        }
        else if (key == "colormap_entries")
        {
            image.colormap_entries = helper::StringTo<uint32_t>(
                value, m_DebugMode,
                " when setting colormap_entries in ADIOS2 PNG compressor");
        }
    }
    return image;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
