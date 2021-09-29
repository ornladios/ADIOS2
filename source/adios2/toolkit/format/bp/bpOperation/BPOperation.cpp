/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BPOperation.cpp :
 *
 *  Created on: Jul 20, 2018
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BPOperation.h"
#include "BPOperation.tcc"

namespace adios2
{
namespace format
{

#define declare_type(T)                                                        \
                                                                               \
    template void BPOperation::SetData(                                        \
        const core::Variable<T> &, const typename core::Variable<T>::BPInfo &, \
        const typename core::Variable<T>::Operation &, BufferSTL &bufferSTL)   \
        const noexcept;                                                        \
                                                                               \
    template void BPOperation::SetMetadata(                                    \
        const core::Variable<T> &, const typename core::Variable<T>::BPInfo &, \
        const typename core::Variable<T>::Operation &, std::vector<char> &)    \
        const noexcept;                                                        \
                                                                               \
    template void BPOperation::UpdateMetadata(                                 \
        const core::Variable<T> &, const typename core::Variable<T>::BPInfo &, \
        const typename core::Variable<T>::Operation &, std::vector<char> &)    \
        const noexcept;

ADIOS2_FOREACH_STDTYPE_1ARG(declare_type)
#undef declare_type

} // end namespace format
} // end namespace adios2
