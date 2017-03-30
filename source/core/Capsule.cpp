/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Capsule.cpp
 *
 *  Created on: Nov 11, 2016
 *      Author: wfg
 */

#include "core/Capsule.h"

namespace adios
{

Capsule::Capsule(const std::string type, const std::string accessMode,
                 const int rankMPI, const bool debugMode)
    : m_Type{type}, m_AccessMode{accessMode}, m_RankMPI{rankMPI},
      m_DebugMode{debugMode}
{
}

Capsule::~Capsule() {}

void Capsule::ResizeData(const std::size_t size) {}

void Capsule::ResizeMetadata(const std::size_t size) {}

} // end namespace
