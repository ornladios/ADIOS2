/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Capsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */

#include "Capsule.h"

namespace adios
{

Capsule::Capsule(const std::string type, const bool debugMode)
: m_Type(type), m_DebugMode(debugMode)
{
}

size_t Capsule::GetAvailableDataSize() const
{
    return GetDataSize() - m_DataPosition;
}

void Capsule::ResizeData(size_t /*size*/) {}

void Capsule::ResizeMetadata(size_t /*size*/) {}

} // end namespace adios
