/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Zfp.tcc :
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4ZFP_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4ZFP_TCC_

#include "BP4Zfp.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BP4Zfp::SetDataCommon(
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
void BP4Zfp::SetMetadataCommon(
    const core::Variable<T> &variable,
    const typename core::Variable<T>::Info &blockInfo,
    const typename core::Variable<T>::Operation &operation,
    std::vector<char> &buffer) const noexcept
{
    const uint64_t inputSize =
        helper::GetTotalSize(blockInfo.Count) * sizeof(T);
    // being naughty here
    Params &info = const_cast<Params &>(operation.Info);
    info["InputSize"] = std::to_string(inputSize);

    const uint64_t outputSize = 0; // not known yet

    auto itMode = operation.Parameters.find("accuracy");
    int32_t mode = -1;

    if (itMode != operation.Parameters.end())
    {
        mode = static_cast<int32_t>(zfp_mode_accuracy);
    }
    else
    {
        itMode = operation.Parameters.find("precision");
        if (itMode != operation.Parameters.end())
        {
            mode = static_cast<int32_t>(zfp_mode_precision);
        }
        itMode = operation.Parameters.find("rate");
        if (itMode != operation.Parameters.end())
        {
            mode = static_cast<int32_t>(zfp_mode_rate);
        }
    }
    const std::string modeStr = itMode->second;

    // fixed size
    constexpr uint16_t metadataSize = 532;
    helper::InsertToBuffer(buffer, &metadataSize);
    helper::InsertToBuffer(buffer, &inputSize);
    // to be filled out after operation is applied on data
    info["OutputSizeMetadataPosition"] = std::to_string(buffer.size());
    helper::InsertToBuffer(buffer, &outputSize);
    helper::InsertToBuffer(buffer, &mode);

    const size_t fixedRecordsPosition = buffer.size();
    buffer.resize(fixedRecordsPosition + 512, '\0');
    size_t backPosition = fixedRecordsPosition;
    helper::CopyToBuffer(buffer, backPosition, modeStr.data(), modeStr.size());
    backPosition = fixedRecordsPosition + 256;
    helper::CopyToBuffer(buffer, backPosition, variable.m_Name.data(),
                         variable.m_Name.size());
}

template <class T>
void BP4Zfp::UpdateMetadataCommon(
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

#endif /* ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4ZFP_H_ */
