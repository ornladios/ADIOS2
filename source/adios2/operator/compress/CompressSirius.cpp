/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * CompressSirius.cpp
 *
 *  Created on: Jul 28, 2021
 *      Author: Jason Wang jason.ruonan.wang@gmail.com
 */

#include "CompressSirius.h"
#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace compress
{

CompressSirius::CompressSirius(const Params &parameters) : Operator("sirius", parameters) {}

CompressSirius::~CompressSirius()
{}

size_t CompressSirius::Compress(const void *dataIn, const Dims &dimensions,
                            const size_t elementSize, DataType varType,
                            void *bufferOut, const Params &parameters,
                            Params &info) const
{
    return 0;
}

size_t CompressSirius::Decompress(const void *bufferIn, const size_t sizeIn,
                              void *dataOut, const Dims &dimensions,
                              DataType varType,
                              const Params & /*parameters*/) const
{
    return 0;
}

bool CompressSirius::IsDataTypeValid(const DataType type) const
{
    if (helper::GetDataType<float>() == type)
    {
        return true;
    }
    return false;
}

} // end namespace compress
} // end namespace core
} // end namespace adios2
