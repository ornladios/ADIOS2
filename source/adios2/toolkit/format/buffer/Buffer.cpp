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

Buffer::Buffer(const std::string type, const bool debugMode)
: m_Type(type), m_DebugMode(debugMode)
{
}

void Buffer::Resize(const size_t size, const std::string hint)
{
    throw std::invalid_argument("ERROR: buffer memory of type " + m_Type +
                                " can't call Resize " + hint + "\n");
}

} // end namespace format
} // end namespace adios2
