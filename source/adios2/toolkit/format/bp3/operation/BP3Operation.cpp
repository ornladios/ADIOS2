/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP3Operation.cpp :
 *
 *  Created on: Jul 20, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP3Operation.h"

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
    void BP3Operation::SetData(                                                \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        BufferSTL &bufferSTL) const noexcept                                   \
    {                                                                          \
    }                                                                          \
                                                                               \
    void BP3Operation::SetMetadata(                                            \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
    }                                                                          \
                                                                               \
    void BP3Operation::UpdateMetadata(                                         \
        const core::Variable<T> &variable,                                     \
        const typename core::Variable<T>::Info &blockInfo,                     \
        const typename core::Variable<T>::Operation &operation,                \
        std::vector<char> &buffer) const noexcept                              \
    {                                                                          \
    }

ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type

} // end namespace format
} // end namespace adios2
