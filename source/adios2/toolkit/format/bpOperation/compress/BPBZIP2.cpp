/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPBZIP2.cpp
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPBZIP2.h"
#include "BPBZIP2.tcc"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_BZIP2
#include "adios2/operator/compress/CompressBZIP2.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BPBZIP2::SetData(                                                     \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPBZIP2::SetMetadata(                                                 \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataCommon(variable, blockInfo, operation, buffer);             \
    }                                                                          \
                                                                               \
    void BPBZIP2::UpdateMetadata(                                              \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataCommon(variable, blockInfo, operation, buffer);          \
    }

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

void BPBZIP2::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));

    const uint16_t batches = helper::ReadValue<uint16_t>(buffer, position);
    info["batches"] = std::to_string(batches);

    for (auto b = 0; b < batches; ++b)
    {
        const std::string bStr = std::to_string(b);

        info["OriginalOffset_" + bStr] =
            std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["OriginalSize_" + bStr] =
            std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["CompressedOffset_" + bStr] =
            std::to_string(helper::ReadValue<uint64_t>(buffer, position));
        info["CompressedSize_" + bStr] =
            std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    }
}

void BPBZIP2::GetData(const char *input,
                      const helper::BlockOperationInfo &blockOperationInfo,
                      char *dataOutput) const
{
#ifdef ADIOS2_HAVE_BZIP2
    core::compress::CompressBZIP2 op(Params(), true);
    const size_t sizeOut = (sizeof(size_t) == 8)
                               ? static_cast<size_t>(helper::StringTo<uint64_t>(
                                     blockOperationInfo.Info.at("InputSize"),
                                     true, "when reading BZIP2 input size"))
                               : static_cast<size_t>(helper::StringTo<uint32_t>(
                                     blockOperationInfo.Info.at("InputSize"),
                                     true, "when reading BZIP2 input size"));

    Params &info = const_cast<Params &>(blockOperationInfo.Info);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput, sizeOut,
                  info);

#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with BZIP2, can't read BZIP2 compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
