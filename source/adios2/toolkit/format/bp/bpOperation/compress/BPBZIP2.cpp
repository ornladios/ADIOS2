/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPBZIP2.cpp
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPBZIP2.h"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZIP2.h"
#endif

namespace adios2
{
namespace format
{

void BPBZIP2::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPBZIP2::GetData(const char *input,
                      const helper::BlockOperationInfo &blockOperationInfo,
                      char *dataOutput) const
{
#ifdef ADIOS2_HAVE_BZIP2
    core::compress::CompressBZIP2 op({});
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with BZIP2, can't read BZIP2 compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
