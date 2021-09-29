/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPZFP.cpp :
 *
 *  Created on: Jul 17, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPZFP.h"

#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosType.h"

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZFP.h"
#endif

namespace adios2
{
namespace format
{

void BPZFP::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPZFP::GetData(const char *input,
                    const helper::BlockOperationInfo &blockOperationInfo,
                    char *dataOutput) const
{
#ifdef ADIOS2_HAVE_ZFP
    core::compress::CompressZFP op({});
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with ZFP, can't read ZFP compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
