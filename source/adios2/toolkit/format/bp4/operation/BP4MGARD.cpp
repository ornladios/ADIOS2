/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4MGARD.cpp
 *
 *  Created on: Jan 19, 2019
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4MGARD.h"
#include "BP4MGARD.tcc"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_MGARD
#include "adios2/operator/compress/CompressMGARD.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BP4MGARD::SetData(                                                    \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataCommon(variable, blockInfo, operation, bufferSTL);              \
    }                                                                          \
                                                                               \
    void BP4MGARD::SetMetadata(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataCommon(variable, blockInfo, operation, buffer);             \
    }                                                                          \
                                                                               \
    void BP4MGARD::UpdateMetadata(                                             \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataCommon(variable, blockInfo, operation, buffer);          \
    }

ADIOS2_FOREACH_MGARD_TYPE_1ARG(declare_type)
#undef declare_type

void BP4MGARD::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BP4MGARD::GetData(const char *input,
                       const helper::BlockOperationInfo &blockOperationInfo,
                       char *dataOutput) const
{
#ifdef ADIOS2_HAVE_MGARD
    core::compress::CompressMGARD op(Params(), true);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount, blockOperationInfo.PreDataType,
                  blockOperationInfo.Info);

#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with MGARD, can't read MGARD compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
