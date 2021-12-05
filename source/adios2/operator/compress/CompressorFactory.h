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
#include "adios2/core/Operator.h"

namespace adios2
{
namespace core
{
namespace compress
{

std::shared_ptr<Operator> MakeOperator(const std::string &type,
                                       const Params &parameters);

size_t Compress(const char *dataIn, const Dims &blockStart,
                const Dims &blockCount, const DataType type, char *bufferOut,
                const Params &parameters, const std::string &compressorType);

size_t Decompress(const char *bufferIn, const size_t sizeIn, char *dataOut);

} // end namespace compress
} // end namespace core
} // end namespace adios2
