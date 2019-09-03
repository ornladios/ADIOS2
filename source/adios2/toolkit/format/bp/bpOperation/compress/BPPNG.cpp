/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPPNG.cpp
 *
 *  Created on: Jun 10, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPPNG.h"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_PNG
#include "adios2/operator/compress/CompressPNG.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BPPNG::SetData(                                                       \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPPNG::SetMetadata(                                                   \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataDefault(variable, blockInfo, operation, buffer);            \
    }                                                                          \
                                                                               \
    void BPPNG::UpdateMetadata(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataDefault(variable, blockInfo, operation, buffer);         \
    }

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

void BPPNG::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
}

void BPPNG::GetData(const char *input,
                    const helper::BlockOperationInfo &blockOperationInfo,
                    char *dataOutput) const
{
#ifdef ADIOS2_HAVE_PNG
    core::compress::CompressPNG op(Params(), true);
    const size_t sizeOut = (sizeof(size_t) == 8)
                               ? static_cast<size_t>(helper::StringTo<uint64_t>(
                                     blockOperationInfo.Info.at("InputSize"),
                                     true, "when reading PNG input size"))
                               : static_cast<size_t>(helper::StringTo<uint32_t>(
                                     blockOperationInfo.Info.at("InputSize"),
                                     true, "when reading PNG input size"));

    Params &info = const_cast<Params &>(blockOperationInfo.Info);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput, sizeOut,
                  info);

#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with PNG, can't read PNG compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
