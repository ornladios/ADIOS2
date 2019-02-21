/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Zfp.cpp :
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4SZ.h"
#include "BP4SZ.tcc"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_SZ
#include "adios2/operator/compress/CompressSZ.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BP4SZ::SetData(                                                       \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataCommon(variable, blockInfo, operation, bufferSTL);              \
    }                                                                          \
                                                                               \
    void BP4SZ::SetMetadata(                                                   \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataCommon(variable, blockInfo, operation, buffer);             \
    }                                                                          \
                                                                               \
    void BP4SZ::UpdateMetadata(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataCommon(variable, blockInfo, operation, buffer);          \
    }

ADIOS2_FOREACH_SZ_TYPE_1ARG(declare_type)
#undef declare_type

void BP4SZ::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BP4SZ::GetData(const char *input,
                    const helper::BlockOperationInfo &blockOperationInfo,
                    char *dataOutput) const
{
#ifdef ADIOS2_HAVE_SZ
    core::compress::CompressSZ op(Params(), true);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount,
                  DataType(blockOperationInfo.Info.at("PreDataType")),
                  blockOperationInfo.Info);

#else
    throw std::runtime_error("ERROR: current ADIOS2 library didn't compile "
                             "with SZ, can't read SZ compressed data, in call "
                             "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
