/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Operation.h :
 *
 *  Created on: Jul 12, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3OPERATION_H_
#define ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3OPERATION_H_

#include <string>
#include <vector>

#include "adios2/ADIOSMacros.h"
#include "adios2/core/Variable.h"
#include "adios2/helper/adiosFunctions.h"
#include "adios2/toolkit/format/BufferSTL.h"

namespace adios2
{
namespace format
{

class BP3Operation
{
public:
    BP3Operation() = default;
    virtual ~BP3Operation() = default;

#define declare_type(T)                                                        \
    virtual void SetData(                                                      \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept;                                  \
                                                                               \
    virtual void SetMetadata(                                                  \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept;                             \
                                                                               \
    virtual void UpdateMetadata(                                               \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept;

    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

    /**
     * Deserializes metadata in the form of parameters
     * @param buffer contains serialized metadata buffer
     * @param info parameters info from metadata buffer
     */
    virtual void GetMetadata(const std::vector<char> &buffer,
                             Params &info) const noexcept = 0;

    virtual void GetData(const char *input,
                         const helper::BlockOperationInfo &blockOperationInfo,
                         char *dataOutput) const = 0;
};

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BP3_OPERATION_BP3OPERATION_H_ */
