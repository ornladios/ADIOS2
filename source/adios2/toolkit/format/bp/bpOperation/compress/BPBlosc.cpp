/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPBlosc.cpp
 *
 *  Created on: Jun 21, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPBlosc.h"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_BLOSC
#include "adios2/operator/compress/CompressBlosc.h"
#endif

namespace adios2
{
namespace format
{

void BPBlosc::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPBlosc::GetData(const char *input,
                      const helper::BlockOperationInfo &blockOperationInfo,
                      char *dataOutput) const
{
#ifdef ADIOS2_HAVE_BLOSC
    core::compress::CompressBlosc op({});
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with Blosc, can't read Blosc compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
