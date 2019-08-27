/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * NullEngine.cpp : Null engine does nothing, used for benchmarking
 *
 *  Created on: Mar 11, 2019
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "NullEngine.h"

namespace adios2
{
namespace core
{
namespace engine
{

NullEngine::NullEngine(IO &io, const std::string &name, const Mode mode,
                       helper::Comm comm)
: Engine("NULL", io, name, mode, std::move(comm))
{
}

// PRIVATE
void NullEngine::DoClose(const int /*transportIndex*/) {}

} // end namespace engine
} // end namespace core
} // end namespace adios2
