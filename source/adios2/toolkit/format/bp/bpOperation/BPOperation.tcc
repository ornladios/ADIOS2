/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPOperation.tcc
 *
 *  Created on: Jun 11, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_BPOPERATION_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_BPOPERATION_TCC_

#include "BPOperation.h"

namespace adios2
{
namespace format
{
// DEFAULTS only saves input and output payload sizes in metadata
// PROTECTED
template <class T>
void BPOperation::SetDataDefault(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    BufferSTL &bufferSTL) const noexcept
{
    const core::Operator &op = *operation.Op;
    const Params &parameters = operation.Parameters;
    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);

    const size_t outputSize = op.Compress(
        blockInfo.Data, blockInfo.Count, variable.m_ElementSize,
        variable.m_Type, bufferSTL.m_Buffer.data() + bufferSTL.m_Position,
        parameters, info);

    info["OutputSize"] = std::to_string(outputSize);

    bufferSTL.m_Position += outputSize;
    bufferSTL.m_AbsolutePosition += outputSize;
}

template <class T>
void BPOperation::SetMetadataDefault(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    std::vector<char> &buffer) const noexcept
{
    const uint64_t inputSize = static_cast<uint64_t>(
        helper::GetTotalSize(blockInfo.Count) * sizeof(T));
    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);
    info["InputSize"] = std::to_string(inputSize);

    // fixed size only stores inputSize 8-bytes and outputSize 8-bytes
    constexpr uint16_t metadataSize = 16;
    helper::InsertToBuffer(buffer, &metadataSize);
    helper::InsertToBuffer(buffer, &inputSize);
    info["OutputSizeMetadataPosition"] = std::to_string(buffer.size());
    constexpr uint64_t outputSize = 0;
    helper::InsertToBuffer(buffer, &outputSize);
}

template <class T>
void BPOperation::UpdateMetadataDefault(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    std::vector<char> &buffer) const noexcept
{
    const uint64_t outputSize =
        static_cast<uint64_t>(std::stoll(operation.Info.at("OutputSize")));

    size_t backPosition = static_cast<size_t>(
        std::stoll(operation.Info.at("OutputSizeMetadataPosition")));

    helper::CopyToBuffer(buffer, backPosition, &outputSize);

    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);
    info.erase("OutputSizeMetadataPosition");
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_BPOPERATION_TCC_ */
