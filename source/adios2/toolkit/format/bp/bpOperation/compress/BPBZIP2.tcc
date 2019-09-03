/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPBZIP2.tcc
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPBZIP2_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPBZIP2_TCC_

#include "BPBZIP2.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BPBZIP2::SetMetadataCommon(
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
    const uint16_t batches =
        static_cast<uint16_t>(inputSize / DefaultMaxFileBatchSize + 1);

    const uint16_t metadataSize = 8 + 8 + 2 + batches * (4 * 8);
    helper::InsertToBuffer(buffer, &metadataSize);
    helper::InsertToBuffer(buffer, &inputSize);
    info["OutputSizeMetadataPosition"] = std::to_string(buffer.size());

    constexpr uint64_t outputSize = 0;
    // dummy
    helper::InsertToBuffer(buffer, &outputSize);
    // additional batch info
    helper::InsertToBuffer(buffer, &batches);
    info["BatchesMetadataPosition"] = std::to_string(buffer.size());

    // inserting dummies to preallocate, updated in UpdateMetadataCommon
    buffer.resize(buffer.size() + batches * 4 * 8);
}

template <class T>
void BPBZIP2::UpdateMetadataCommon(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    std::vector<char> &buffer) const noexcept
{
    const uint64_t inputSize = static_cast<uint64_t>(
        helper::GetTotalSize(blockInfo.Count) * sizeof(T));

    const uint64_t outputSize =
        static_cast<uint64_t>(std::stoll(operation.Info.at("OutputSize")));

    size_t backPosition = static_cast<size_t>(
        std::stoull(operation.Info.at("OutputSizeMetadataPosition")));

    helper::CopyToBuffer(buffer, backPosition, &outputSize);
    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);

    // additional metadata for supporting 64-bit count
    const uint16_t batches =
        static_cast<uint16_t>(inputSize / DefaultMaxFileBatchSize + 1);

    backPosition = static_cast<size_t>(
        std::stoull(operation.Info.at("BatchesMetadataPosition")));

    for (auto b = 0; b < batches; ++b)
    {
        const std::string bStr = std::to_string(b);

        const uint64_t originalOffset =
            std::stoull(info["OriginalOffset_" + bStr]);
        const uint64_t originalSize = std::stoull(info["OriginalSize_" + bStr]);
        const uint64_t compressedOffset =
            std::stoull(info["CompressedOffset_" + bStr]);
        const uint64_t compressedSize =
            std::stoull(info["CompressedSize_" + bStr]);

        helper::CopyToBuffer(buffer, backPosition, &originalOffset);
        helper::CopyToBuffer(buffer, backPosition, &originalSize);
        helper::CopyToBuffer(buffer, backPosition, &compressedOffset);
        helper::CopyToBuffer(buffer, backPosition, &compressedSize);
    }

    info.erase("OutputSizeMetadataPosition");
    info.erase("BatchesMetadataPosition");
}

} // end namespace format
} // end namespace adios2

#endif /** ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPBZIP2_TCC_ */
