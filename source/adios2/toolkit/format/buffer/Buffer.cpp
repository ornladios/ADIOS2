/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Buffer.cpp
 *
 *  Created on: Jul 9, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Buffer.h"

namespace adios2
{
namespace format
{

Buffer::Buffer(const std::string type, const size_t fixedSize)
: m_Type(type), m_FixedSize(fixedSize)
{
}

void Buffer::Resize(const size_t size, const std::string hint)
{
    throw std::invalid_argument("ERROR: buffer memory of type " + m_Type +
                                " can't call Resize " + hint + "\n");
}

void Buffer::Reset(const bool resetAbsolutePosition, const bool zeroInitialize)
{
    throw std::invalid_argument("ERROR: buffer memory of type " + m_Type +
                                " can't call Reset\n");
}

char *Buffer::Data() noexcept { return nullptr; }

const char *Buffer::Data() const noexcept { return nullptr; }

size_t Buffer::GetAvailableSize() const
{
    if (m_FixedSize > 0 && m_FixedSize >= m_Position)
    {
        return m_FixedSize - m_Position;
    }
    return 0;
}

} // end namespace format
} // end namespace adios2
