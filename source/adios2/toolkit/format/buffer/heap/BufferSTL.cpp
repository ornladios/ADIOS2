/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferSTL.cpp
 *
 *  Created on: Sep 26, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BufferSTL.h"
#include "BufferSTL.tcc"

namespace adios2
{
namespace format
{

BufferSTL::BufferSTL() : Buffer("BufferSTL") {}

char *BufferSTL::Data() noexcept { return m_Buffer.data(); }

const char *BufferSTL::Data() const noexcept { return m_Buffer.data(); }

void BufferSTL::Resize(const size_t size, const std::string hint)
{
    try
    {
        // doing this will effectively replace the STL GNU default power of 2
        // reallocation.
        m_Buffer.reserve(size);
        // must initialize memory (secure)
        m_Buffer.resize(size, '\0');
    }
    catch (...)
    {
        // catch a bad_alloc
        std::throw_with_nested(std::runtime_error(
            "ERROR: buffer overflow when resizing to " + std::to_string(size) +
            " bytes, " + hint + "\n"));
    }
}

void BufferSTL::Reset(const bool resetAbsolutePosition,
                      const bool zeroInitialize)
{
    m_Position = 0;
    if (resetAbsolutePosition)
    {
        m_AbsolutePosition = 0;
    }
    if (zeroInitialize)
    {
        m_Buffer.assign(m_Buffer.size(), '\0');
    }
}

size_t BufferSTL::GetAvailableSize() const
{
    return m_Buffer.size() - m_Position;
}

#define declare_template_instantiation(T)                                      \
    template size_t BufferSTL::Align<T>() const noexcept;

ADIOS2_FOREACH_PRIMITIVE_STDTYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
