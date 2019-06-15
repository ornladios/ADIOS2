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

#ifdef ADIOS2_HAVE_MGARD
#include "adios2/operator/compress/CompressMGARD.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BPMGARD::SetData(                                                     \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPMGARD::SetMetadata(                                                 \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataDefault(variable, blockInfo, operation, buffer);            \
    }                                                                          \
                                                                               \
    void BPMGARD::UpdateMetadata(                                              \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataDefault(variable, blockInfo, operation, buffer);         \
    }

ADIOS2_FOREACH_MGARD_TYPE_1ARG(declare_type)
#undef declare_type

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
    core::compress::CompressMGARD op(Params(), true);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount,
                  blockOperationInfo.Info.at("PreDataType"),
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
