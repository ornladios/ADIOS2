/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4MGARD.h
 *
 *  Created on: Jan 19, 2019
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4MGARD_H_
#define ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4MGARD_H_

#include "adios2/toolkit/format/bp4/operation/BP4Operation.h"

namespace adios2
{
namespace format
{

class BP4MGARD : public BP4Operation
{
public:
    BP4MGARD() = default;

    ~BP4MGARD() = default;

    using BP4Operation::SetData;
    using BP4Operation::SetMetadata;
    using BP4Operation::UpdateMetadata;
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

    ADIOS2_FOREACH_MGARD_TYPE_1ARG(declare_type)
#undef declare_type

    void GetMetadata(const std::vector<char> &buffer, Params &info) const
        noexcept final;

    void GetData(const char *input,
                 const helper::BlockOperationInfo &blockOperationInfo,
                 char *dataOutput) const final;

private:
    template <class T>
    void SetDataCommon(const core::Variable<T> &variable,
                       const typename core::Variable<T>::Info &blockInfo,
                       const typename core::Variable<T>::Operation &operation,
                       BufferSTL &bufferSTL) const noexcept;

    template <class T>
    void GetDataCommon(const char *input,
                       const helper::BlockOperationInfo &blockOperationInfo,
                       T *dataOutput) const;

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

#endif /* ADIOS2_TOOLKIT_FORMAT_BP4_OPERATION_BP4MGARD_H_ */
