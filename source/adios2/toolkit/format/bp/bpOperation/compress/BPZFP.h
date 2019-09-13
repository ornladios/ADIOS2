/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPZFP.h :
 *
 *  Created on: Jul 17, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPZFP_H_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPZFP_H_

#include "adios2/toolkit/format/bp/bpOperation/BPOperation.h"

namespace adios2
{
namespace format
{

class BPZFP : public BPOperation
{
public:
    BPZFP() = default;

    ~BPZFP() = default;

    using BPOperation::SetData;
    using BPOperation::SetMetadata;
    using BPOperation::UpdateMetadata;
#define declare_type(T)                                                        \
    void SetData(const core::Variable<T> &variable,                            \
                 const typename core::Variable<T>::Info &blockInfo,            \
                 const typename core::Variable<T>::Operation &operation,       \
                 BufferSTL &bufferSTL) const noexcept override;                \
                                                                               \
    void SetMetadata(const core::Variable<T> &variable,                        \
                     const typename core::Variable<T>::Info &blockInfo,        \
                     const typename core::Variable<T>::Operation &operation,   \
                     std::vector<char> &buffer) const noexcept override;       \
                                                                               \
    void UpdateMetadata(                                                       \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept override;

    ADIOS2_FOREACH_ZFP_TYPE_1ARG(declare_type)
#undef declare_type

    void GetMetadata(const std::vector<char> &buffer, Params &info) const
        noexcept final;

    void GetData(const char *input,
                 const helper::BlockOperationInfo &blockOperationInfo,
                 char *dataOutput) const final;

private:
    enum Mode
    {
        zfp_mode_accuracy = 0,
        zfp_mode_precision = 1,
        zfp_mode_rate = 2
    };

    template <class T>
    void
    SetMetadataCommon(const core::Variable<T> &variable,
                      const typename core::Variable<T>::Info &blockInfo,
                      const typename core::Variable<T>::Operation &operation,
                      std::vector<char> &buffer) const noexcept;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_COMPRESS_BPZFP_H_ */
