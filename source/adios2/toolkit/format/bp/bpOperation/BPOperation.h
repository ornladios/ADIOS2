/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPOperation.h :
 *
 *  Created on: Jul 12, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_BPOPERATION_H_
#define ADIOS2_TOOLKIT_FORMAT_BP_BPOPERATION_BPOPERATION_H_

#include <string>
#include <vector>

#include "adios2/common/ADIOSMacros.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/format/buffer/heap/BufferSTL.h"

namespace adios2
{
namespace format
{

class BPOperation
{
public:
    BPOperation() = default;
    virtual ~BPOperation() = default;

    /**
     * Deserializes metadata in the form of parameters
     * @param buffer contains serialized metadata buffer
     * @param info parameters info from metadata buffer
     */
    void GetMetadata(const std::vector<char> &buffer, Params &info) const
        noexcept;

    template <class T>
    void SetData(const core::Variable<T> &variable,
                 const typename core::Variable<T>::BPInfo &blockInfo,
                 const typename core::Variable<T>::Operation &operation,
                 BufferSTL &bufferSTL) const noexcept;

    template <class T>
    void SetMetadata(const core::Variable<T> &variable,
                     const typename core::Variable<T>::BPInfo &blockInfo,
                     const typename core::Variable<T>::Operation &operation,
                     std::vector<char> &buffer) const noexcept;

    template <class T>
    void UpdateMetadata(const core::Variable<T> &variable,
                        const typename core::Variable<T>::BPInfo &blockInfo,
                        const typename core::Variable<T>::Operation &operation,
                        std::vector<char> &buffer) const noexcept;
};

#define declare_type(T)                                                        \
                                                                               \
    extern template void BPOperation::SetData(                                 \
        const core::Variable<T> &, const typename core::Variable<T>::BPInfo &, \
        const typename core::Variable<T>::Operation &, BufferSTL &bufferSTL)   \
        const noexcept;                                                        \
                                                                               \
    extern template void BPOperation::SetMetadata(                             \
        const core::Variable<T> &, const typename core::Variable<T>::BPInfo &, \
        const typename core::Variable<T>::Operation &, std::vector<char> &)    \
        const noexcept;                                                        \
                                                                               \
    extern template void BPOperation::UpdateMetadata(                          \
        const core::Variable<T> &, const typename core::Variable<T>::BPInfo &, \
        const typename core::Variable<T>::Operation &, std::vector<char> &)    \
        const noexcept;

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP_OPERATION_BPOPERATION_H_ */
