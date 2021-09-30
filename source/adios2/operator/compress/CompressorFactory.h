/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressorFactory.h :
 *
 *  Created on: Sep 29, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "adios2/common/ADIOSTypes.h"

#ifdef ADIOS2_HAVE_BLOSC
#include "adios2/operator/compress/CompressBlosc.h"
#endif

#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZIP2.h"
#endif

#ifdef ADIOS2_HAVE_LIBPRESSIO
#include "adios2/operator/compress/CompressLibPressio.h"
#endif

#ifdef ADIOS2_HAVE_MGARD
#include "adios2/operator/compress/CompressMGARD.h"
#endif

#ifdef ADIOS2_HAVE_PNG
#include "adios2/operator/compress/CompressPNG.h"
#endif

#ifdef ADIOS2_HAVE_MHS
#include "adios2/operator/compress/CompressSirius.h"
#endif

#ifdef ADIOS2_HAVE_SZ
#include "adios2/operator/compress/CompressSZ.h"
#endif

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZFP.h"
#endif

namespace adios2
{
namespace core
{
namespace compress
{

class CompressorFactory
{
public:
    size_t Compress(const char *dataIn, const Dims &blockStart,
                    const Dims &blockCount, const DataType type,
                    char *bufferOut, const Params &parameters,
                    const std::string &compressorType);

    size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut);
};

} // end namespace compress
} // end namespace core
} // end namespace adios2
