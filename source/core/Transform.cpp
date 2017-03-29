/*
 * Transform.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: wfg
 */

#include "core/Transform.h"

namespace adios
{

Transform::Transform(const std::string method) : m_Method(method) {}

Transform::~Transform() {}

void Transform::Compress(const std::vector<char> &bufferIn,
                         std::vector<char> &bufferOut)
{
}

void Transform::Decompress(const std::vector<char> &bufferIn,
                           std::vector<char> &bufferOut)
{
}

} // end namespace
