/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPLIBPRESSIO.cpp :
 *
 *  Created on: Apr 13, 2021
 *      Author: Robert Underwood
 */

#include "BPLIBPRESSIO.h"

#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosType.h"

#ifdef ADIOS2_HAVE_LIBPRESSIO
#include "adios2/operator/compress/CompressLibPressio.h"
#endif

namespace adios2
{
namespace format
{

void BPLIBPRESSIO::GetData(const char *input,
                           const helper::BlockOperationInfo &blockOperationInfo,
                           char *dataOutput) const
{
#ifdef ADIOS2_HAVE_LIBPRESSIO
    core::compress::CompressLibPressio op({});
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput);
#else
    throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                             "with SZ, can't read SZ compressed data, in call "
                             "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
