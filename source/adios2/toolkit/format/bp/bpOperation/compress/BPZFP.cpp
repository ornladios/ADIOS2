/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPZFP.cpp :
 *
 *  Created on: Jul 17, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPZFP.h"
#include "BPZFP.tcc"

#include "adios2/helper/adiosFunctions.h"

#ifdef ADIOS2_HAVE_ZFP
#include "adios2/operator/compress/CompressZFP.h"
#endif

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BPZFP::SetData(                                                       \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
        SetDataDefault(variable, blockInfo, operation, bufferSTL);             \
    }                                                                          \
                                                                               \
    void BPZFP::SetMetadata(                                                   \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        SetMetadataCommon(variable, blockInfo, operation, buffer);             \
    }                                                                          \
                                                                               \
    void BPZFP::UpdateMetadata(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
        UpdateMetadataDefault(variable, blockInfo, operation, buffer);         \
    }

ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type

void BPZFP::GetMetadata(const std::vector<char> &buffer, Params &info) const
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

void BPZFP::GetData(const char *input,
                    const helper::BlockOperationInfo &blockOperationInfo,
                    char *dataOutput) const
{
#ifdef ADIOS2_HAVE_ZFP
    core::compress::CompressZFP op(Params(), true);
    op.Decompress(input, blockOperationInfo.PayloadSize, dataOutput,
                  blockOperationInfo.PreCount,
                  blockOperationInfo.Info.at("PreDataType"),
                  blockOperationInfo.Info);
#else
    throw std::runtime_error(
        "ERROR: current ADIOS2 library didn't compile "
        "with ZFP, can't read ZFP compressed data, in call "
        "to Get\n");
#endif
}

} // end namespace format
} // end namespace adios2
