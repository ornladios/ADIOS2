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

#include <utility>

namespace adios
{

Capsule::Capsule(const std::string type, const std::string accessMode,
                 int rankMPI, bool debugMode)
: m_Type{std::move(type)}, m_AccessMode{std::move(accessMode)},
  m_RankMPI{rankMPI}, m_DebugMode{debugMode}
{
}

void Capsule::ResizeData(size_t /*size*/) {}

void Capsule::ResizeMetadata(size_t /*size*/) {}

} // end namespace adios
