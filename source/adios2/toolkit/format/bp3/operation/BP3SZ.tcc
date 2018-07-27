/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3SZ.tcc :
 *
 *  Created on: Jul 18, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3SZ_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3SZ_TCC_

#include "BP3SZ.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BP3SZ::SetDataCommon(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    BufferSTL &bufferSTL) const noexcept
{
    const core::Operator &op = *operation.Op;
    const Params &parameters = operation.Parameters;

    const size_t outputSize = op.Compress(
        blockInfo.Data, blockInfo.Count, variable.m_ElementSize,
        variable.m_Type, bufferSTL.m_Buffer.data() + bufferSTL.m_Position,
        parameters);

    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);
    info["OutputSize"] = std::to_string(outputSize);

    bufferSTL.m_Position += outputSize;
    bufferSTL.m_AbsolutePosition += outputSize;
}

template <class T>
void BP3SZ::SetMetadataCommon(
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

    // fixed size
    constexpr uint16_t metadataSize = 16;
    helper::InsertToBuffer(buffer, &metadataSize);
    helper::InsertToBuffer(buffer, &inputSize);
    info["OutputSizeMetadataPosition"] = std::to_string(buffer.size());
    const uint64_t outputSize = 0;
    helper::InsertToBuffer(buffer, &outputSize);
}

template <class T>
void BP3SZ::UpdateMetadataCommon(
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

#endif /* ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3SZ_H_ */
