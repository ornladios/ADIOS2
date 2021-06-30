/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BufferV.cpp
 *
 */

#include "BufferV.h"
#include <string.h>

namespace adios2
{
namespace format
{

BufferV::BufferV(const std::string type, const bool AlwaysCopy)
: m_Type(type), m_AlwaysCopy(AlwaysCopy)
{
}

BufferV::~BufferV() {}

void BufferV::Reset()
{
    CurOffset = 0;
    m_internalPos = 0;
    DataV.clear();
}

uint64_t BufferV::Size() noexcept { return CurOffset; }

} // end namespace format
} // end namespace adios2
