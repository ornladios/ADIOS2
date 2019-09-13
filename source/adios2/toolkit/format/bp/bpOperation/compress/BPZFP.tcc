/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPZFP.tcc :
 *
 *  Created on: Jul 18, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPZFP_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPZFP_TCC_

#include "BPZFP.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace format
{

template <class T>
void BPZFP::SetMetadataCommon(
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
        else
        {
            itMode = operation.Parameters.find("rate");
            if (itMode != operation.Parameters.end())
            {
                mode = static_cast<int32_t>(zfp_mode_rate);
            }
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

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPZFP_TCC_  */
