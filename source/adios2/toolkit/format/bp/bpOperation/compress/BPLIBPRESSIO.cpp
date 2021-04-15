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

#define declare_type(T)                                                        \
    void BPLIBPRESSIO::SetData(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::BPInfo &blockInfo,                   \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPLIBPRESSIO::SetMetadata(                                            \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::BPInfo &blockInfo,                   \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataDefault(variable, blockInfo, operation, buffer);            \
    }                                                                          \
                                                                               \
    void BPLIBPRESSIO::UpdateMetadata(                                         \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::BPInfo &blockInfo,                   \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataDefault(variable, blockInfo, operation, buffer);         \
    }

ADIOS2_FOREACH_LIBPRESSIO_TYPE_1ARG(declare_type)
#undef declare_type

void BPLIBPRESSIO::GetMetadata(const std::vector<char> &buffer,
                               Params &info) const noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPLIBPRESSIO::GetData(const char *input,
                           const helper::BlockOperationInfo &blockOperationInfo,
                           char *dataOutput) const
{
#ifdef ADIOS2_HAVE_LIBPRESSIO
    core::compress::CompressLibPressio op((Params()));
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount,
                  helper::GetDataTypeFromString(
                      blockOperationInfo.Info.at("PreDataType")),
                  blockOperationInfo.Info);

#else
    throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                             "with SZ, can't read SZ compressed data, in call "
                             "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
