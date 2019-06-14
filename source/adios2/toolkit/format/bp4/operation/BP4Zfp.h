/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Zfp.h :
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4ZFP_H_
#define ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4ZFP_H_

#include "adios2/toolkit/format/bp4/operation/BP4Operation.h"

namespace adios2
{
namespace format
{

class BP4Zfp : public BP4Operation
{
public:
    BP4Zfp() = default;

    ~BP4Zfp() = default;

#define declare_type(T)                                                        \
    void SetData(const core::Variable<T> &variable,                            \
                 const typename core::Variable<T>::Info &blockInfo,            \
                 const typename core::Variable<T>::Operation &operation,       \
                 BufferSTL &bufferSTL) const noexcept final;                   \
                                                                               \
    void SetMetadata(const core::Variable<T> &variable,                        \
                     const typename core::Variable<T>::Info &blockInfo,        \
                     const typename core::Variable<T>::Operation &operation,   \
                     std::vector<char> &buffer) const noexcept final;          \
                                                                               \
    void UpdateMetadata(                                                       \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept final;

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

#endif /* ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4ZFP_H_ */
