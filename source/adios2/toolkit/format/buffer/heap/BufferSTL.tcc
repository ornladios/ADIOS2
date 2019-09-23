/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferSTL.tcc
 *
 *  Created on: Sep 18, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#ifndef ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERSTL_TCC_
#define ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERSTL_TCC_

#include "BufferSTL.h"

#include <memory>

namespace adios2
{
namespace format
{

template <class T>
size_t BufferSTL::Align() const noexcept
{
    void *currentAddress = m_Buffer.data() + m_Position;
    size_t size = GetAvailableSize();
    std::align(alignof(T), sizeof(T), currentAddress, size);
    return GetAvailableSize() - size;
}

} // end namespace format
} // end namespace adios2

#endif /* ADIOS2_TOOLKIT_FORMAT_BUFFER_HEAP_BUFFERSTL_TCC_ */
