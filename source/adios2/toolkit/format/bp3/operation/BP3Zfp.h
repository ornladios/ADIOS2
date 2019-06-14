/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Zfp.h :
 *
 *  Created on: Jul 17, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3ZFP_H_
#define ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3ZFP_H_

#include "adios2/toolkit/format/bp3/operation/BP3Operation.h"

namespace adios2
{
namespace format
{

class BP3Zfp : public BP3Operation
{
public:
    BP3Zfp() = default;

    ~BP3Zfp() = default;

    using BP3Operation::SetData;
    using BP3Operation::SetMetadata;
    using BP3Operation::UpdateMetadata;
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
    void SetDataCommon(const core::Variable<T> &variable,
                       const typename core::Variable<T>::Info &blockInfo,
                       const typename core::Variable<T>::Operation &operation,
                       BufferSTL &bufferSTL) const noexcept;

    template <class T>
    void
    SetMetadataCommon(const core::Variable<T> &variable,
                      const typename core::Variable<T>::Info &blockInfo,
                      const typename core::Variable<T>::Operation &operation,
                      std::vector<char> &buffer) const noexcept;

    template <class T>
    void
    UpdateMetadataCommon(const core::Variable<T> &variable,
                         const typename core::Variable<T>::Info &blockInfo,
                         const typename core::Variable<T>::Operation &operation,
                         std::vector<char> &buffer) const noexcept;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3ZFP_H_ */
