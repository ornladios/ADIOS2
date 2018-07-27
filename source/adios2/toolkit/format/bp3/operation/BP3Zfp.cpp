/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Zfp.cpp :
 *
 *  Created on: Jul 17, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP3Zfp.h"
#include "BP3Zfp.tcc"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZfp.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BP3Zfp::SetData(                                                      \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataCommon(variable, blockInfo, operation, bufferSTL);              \
    }                                                                          \
                                                                               \
    void BP3Zfp::SetMetadata(                                                  \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataCommon(variable, blockInfo, operation, buffer);             \
    }                                                                          \
                                                                               \
    void BP3Zfp::UpdateMetadata(                                               \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataCommon(variable, blockInfo, operation, buffer);          \
    }

ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type

void BP3Zfp::GetMetadata(const std::vector<char> &buffer, Params &info) const
    noexcept
{
    size_t position = 0;
    info["InputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    info["OutputSize"] =
        std::to_string(helper::ReadValue<uint64_t>(buffer, position));
    const int mode =
        static_cast<int>(helper::ReadValue<uint32_t>(buffer, position));

    const std::string modeStr(buffer.data() + position);

    switch (mode)
    {
    case zfp_mode_precision:
        info["precision"] = modeStr;
        break;

    case zfp_mode_accuracy:
        info["accuracy"] = modeStr;
        break;

    case zfp_mode_rate:
        info["rate"] = modeStr;
        break;
    }
}

void BP3Zfp::GetData(const char *input,
                     const helper::BlockOperationInfo &blockOperationInfo,
                     char *dataOutput) const
{
#ifdef ADIOS2_HAVE_ZFP
    core::compress::CompressZfp op(Params(), true);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount,
                  blockOperationInfo.Info.at("PreDataType"),
                  blockOperationInfo.Info);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with Zfp, can't read Zfp compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
