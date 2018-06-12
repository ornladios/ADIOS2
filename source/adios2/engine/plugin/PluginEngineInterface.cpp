/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * PluginEngineInterface.cxx
 *
 *  Created on: Aug 17, 2017
 *      Author: Chuck Atkins <chuck.atkins@kitware.com>
 */

#include "PluginEngineInterface.h"

namespace adios2
{
namespace core
{
namespace engine
{

PluginEngineInterface::PluginEngineInterface(IO &io, const std::string &name,
                                             const Mode mode, MPI_Comm mpiComm)
: Engine("PluginInterface", io, name, mode, mpiComm)
{
}

} // end namespace engine
} // end namespace core
} // end namespace adios2
