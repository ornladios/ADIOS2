/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPSirius.cpp :
 *
 *  Created on: Jul 31, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "BPSirius.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/helper/adiosType.h"

#ifdef ADIOS2_HAVE_MHS
#include "adios2/operator/compress/CompressSirius.h"
#endif

namespace adios2
{
namespace format
{

void BPSirius::GetData(const char *input,
                       const helper::BlockOperationInfo &blockOperationInfo,
                       char *dataOutput) const
{
#ifdef ADIOS2_HAVE_MHS
    core::compress::CompressSirius op({});
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with MHS, can't read Sirius compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
