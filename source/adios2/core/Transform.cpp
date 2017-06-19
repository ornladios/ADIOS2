/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Transform.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "Transform.h"

namespace adios2
{

Transform::Transform(const std::string method) : m_Method(method) {}

void Transform::Compress(const std::vector<char> & /*bufferIn*/,
                         std::vector<char> & /*bufferOut*/)
{
}

void Transform::Decompress(const std::vector<char> & /*bufferIn*/,
                           std::vector<char> & /*bufferOut*/)
{
}

} // end namespace adios
