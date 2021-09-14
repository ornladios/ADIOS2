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

#define declare_type(T)                                                        \
    void BPSirius::SetData(                                                    \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::BPInfo &blockInfo,                   \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPSirius::SetMetadata(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::BPInfo &blockInfo,                   \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataDefault(variable, blockInfo, operation, buffer);            \
    }                                                                          \
                                                                               \
    void BPSirius::UpdateMetadata(                                             \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::BPInfo &blockInfo,                   \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataDefault(variable, blockInfo, operation, buffer);         \
    }

ADIOS2_FOREACH_SIRIUS_TYPE_1ARG(declare_type)
#undef declare_type

void BPSirius::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPSirius::GetData(const char *input,
                       const helper::BlockOperationInfo &blockOperationInfo,
                       char *dataOutput) const
{
#ifdef ADIOS2_HAVE_MHS
    core::compress::CompressSirius op((Params()));
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreStart, blockOperationInfo.PreCount,
                  helper::GetDataTypeFromString(
                      blockOperationInfo.Info.at("PreDataType")));
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with MHS, can't read Sirius compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
