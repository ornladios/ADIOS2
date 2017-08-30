/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP1Reader.cpp
 *
 *  Created on: Sep 7, 2017
 *      Author: William F Godoy godoywf@ornl.gov
 */

#include "BP1Reader.h"
#include "BP1Reader.tcc"

namespace adios2
{
namespace format
{

BP1Reader::BP1Reader(MPI_Comm mpiComm, const bool debugMode)
: BP1Base(mpiComm, debugMode)
{
}

} // end namespace format
} // end namespace adios2
