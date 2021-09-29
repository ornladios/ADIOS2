/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPMGARD.cpp
 *
 *  Created on: Nov 16, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPMGARD.h"

#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosType.h"

#ifdef ADIOS2_HAVE_MGARD
#include "adios2/operator/compress/CompressMGARD.h"
#endif

namespace adios2
{
namespace format
{

void BPMGARD::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPMGARD::GetData(const char *input,
                      const helper::BlockOperationInfo &blockOperationInfo,
                      char *dataOutput) const
{
#ifdef ADIOS2_HAVE_MGARD
    core::compress::CompressMGARD op({});
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with MGARD, can't read MGARD compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
